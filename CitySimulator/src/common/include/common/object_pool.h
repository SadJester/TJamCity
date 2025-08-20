
#pragma once

namespace tjs::common {

	// TODO: DON`t skip on review: Need to remove block-size, tls-cache size to constructor parameters
	template<typename _T, size_t _BlockSize = 65536u, size_t _TLSCacheSize = 1024u>
	class ObjectPool {
	public:
		using T = _T;
		static constexpr size_t BlockSize = _BlockSize;
		static constexpr size_t TLSCacheSize = _TLSCacheSize;

	public:
		struct pooled_ptr {
			T* ptr { nullptr };
			uint32_t idx { UINT32_MAX }; // global slot index

			explicit operator bool() const noexcept {
				return ptr != nullptr;
			}
			T& operator*() const noexcept {
				return *ptr;
			}
			T* operator->() const noexcept {
				return ptr;
			}

			T* get() const noexcept {
				return ptr;
			}
		};

		ObjectPool() {
#if TJS_SIMULATION_DEBUG
			for (size_t i = 0; i < TLSCacheSize; ++i) {
				assert(tls_cache_.buf[i] == 0);
			}
			assert(tls_cache_.size == 0);
#endif
			T** tbl = nullptr;
			blocks_tbl_.store(tbl, std::memory_order_release);
			blocks_cnt_.store(0, std::memory_order_release);
		}
		~ObjectPool() {
			destroy_all_live();
			_free_all_blocks();
		}

		ObjectPool(const ObjectPool&) = delete;
		ObjectPool& operator=(const ObjectPool&) = delete;
		ObjectPool(ObjectPool&& other) = delete;
		ObjectPool& operator=(ObjectPool&& other) = delete;

		void destroy_all_live() noexcept {
			auto** tbl = alive_tbl_.load(std::memory_order_acquire);
			uint32_t n  = alive_cnt_.load(std::memory_order_acquire);
			for (uint32_t b = 0; b < n; ++b) {
				auto* block = tbl[b];
				for (uint32_t o = 0; o < BlockSize; ++o) {
					uint32_t idx = b * BlockSize + o;
					if (block[o].load(std::memory_order_relaxed)) {
						std::destroy_at(_slot_ptr(idx));
						block[o].store(false, std::memory_order_relaxed);
					}
				}
			}
		}

		// Preallocate capacity (rounded up to whole blocks).
		void reserve(size_t capacity) {
			std::lock_guard<std::mutex> lk(global_lock_);
			size_t need_blocks = (capacity + BlockSize - 1) / BlockSize;
			while (blocks_.size() < need_blocks) {
				_allocate_block_unsafe(); // fills free_list_
			}
		}

		// Acquire an object slot, construct T in-place, return pooled_ptr (idx + pointer).
		template<typename... Args>
		pooled_ptr acquire(Args&&... args) {
			uint32_t idx;
			if (!_tls_pop(idx)) {
				// refill TLS from global list
				_refill_tls_cache();
				if (!_tls_pop(idx)) {
					// global also empty → make a new block
					std::lock_guard<std::mutex> lk(global_lock_);
					if (free_list_.empty()) {
						_allocate_block_unsafe();
					}
					idx = _pop_global_nolock();
				}
			}

			T* p = _slot_ptr(idx);
			std::construct_at(p, std::forward<Args>(args)...);
			set_alive(idx, true);
			return pooled_ptr { p, idx };
		}

		// Destroy object and return slot to pool.
		void release(pooled_ptr pp) {
			if (!pp) {
				return;
			}
			uint32_t idx = pp.idx;
// Safety in debug: validate pointer matches index
#if TJS_SIMULATION_DEBUG
			assert(_slot_ptr(idx) == pp.ptr && "pooled_ptr idx/pointer mismatch");
#endif

			// Call destructor *before* marking free
			std::destroy_at(pp.ptr);
			set_alive(idx, false);
			_tls_push(idx);
		}

		// Access by index (e.g., if you keep ids in your structures).
		T* get(uint32_t idx) noexcept {
			return _slot_ptr(idx);
		}
		const T* get(uint32_t idx) const noexcept {
			return _slot_ptr(idx);
		}

		// Stats
		size_t capacity() const noexcept { return blocks_.size() * BlockSize; }
		size_t live_count() const noexcept { return capacity() - (free_list_size_.load() + _tls_total_cached()); }
		size_t block_count() const noexcept { return blocks_.size(); }

	protected:
		const std::atomic<std::atomic<bool>**>& alive_table() const noexcept { return alive_tbl_; }
		const std::atomic<uint32_t>& alive_cnt() const noexcept { return alive_cnt_; }
		
		T* _slot_ptr(uint32_t idx) const noexcept {
			uint32_t b = idx / BlockSize, o = idx % BlockSize;
			T** tbl = blocks_tbl_.load(std::memory_order_acquire);
			// b is guaranteed to be < published count for valid indices
			return tbl[b] + o;
		}

		uint32_t _index_of_ptr(const T* p) const noexcept {
			T** tbl = blocks_tbl_.load(std::memory_order_acquire);
			uint32_t n = blocks_cnt_.load(std::memory_order_acquire);
			for (uint32_t b = 0; b < n; ++b) {
				const T* base = tbl[b];
				if (p >= base && p < base + BlockSize) {
					return b * BlockSize + static_cast<uint32_t>(p - base);
				}
			}
			return UINT32_MAX;
		}

	private:
		void set_alive(uint32_t idx, bool v) noexcept {
			uint32_t b = idx / BlockSize, o = idx % BlockSize;
			auto** tbl = alive_tbl_.load(std::memory_order_acquire);
			tbl[b][o].store(v, std::memory_order_release);
		}

		bool is_alive(uint32_t idx) const noexcept {
			uint32_t b = idx / BlockSize, o = idx % BlockSize;
			auto** tbl = alive_tbl_.load(std::memory_order_acquire);
			return tbl[b][o].load(std::memory_order_relaxed);
		}

		void _allocate_block_unsafe() {
			// Aligned allocation improves line sharing; requires matching delete.
			T* block = reinterpret_cast<T*>(
				::operator new[](sizeof(T) * BlockSize, std::align_val_t(64)));
			blocks_.push_back(block);

			// alive flags for this block
			auto alive = std::make_unique<std::atomic<bool>[]>(BlockSize);
			for (size_t i = 0; i < BlockSize; ++i) {
				alive[i].store(false, std::memory_order_relaxed);
			}
			alive_blocks_.push_back(std::move(alive));

			// push newly created indices to free list
			uint32_t start_idx = static_cast<uint32_t>((blocks_.size() - 1) * BlockSize);
			for (uint32_t i = 0; i < BlockSize; ++i) {
				free_list_.push_back(start_idx + (BlockSize - 1 - i));
			}
			free_list_size_.store(free_list_.size(), std::memory_order_relaxed);


			 // Publish new block table
			const uint32_t new_cnt = static_cast<uint32_t>(blocks_.size()); // under lock from parent function
			_publish_alive_table_nolock(new_cnt);
			_publish_blocks_table(new_cnt);
		}

		uint32_t _pop_global_nolock() {
			uint32_t id = free_list_.back();
			free_list_.pop_back();
			free_list_size_.store(free_list_.size(), std::memory_order_relaxed);
			return id;
		}

		bool _tls_pop(uint32_t& out) noexcept {
			auto& c = tls_cache_;
			if (c.size == 0) {
				return false;
			}
			out = c.buf[--c.size];
			return true;
		}

		void _tls_push(uint32_t id) {
			auto& c = tls_cache_;
			if (c.size < TLSCacheSize) {
				c.buf[c.size++] = id;
				return;
			}
			// Spill to global in a batch
			std::lock_guard<std::mutex> lk(global_lock_);
			for (uint32_t i = 0; i < c.size; ++i) {
				free_list_.push_back(c.buf[i]);
			}
			free_list_.push_back(id);
			c.size = 0;
			free_list_size_.store(free_list_.size(), std::memory_order_relaxed);
		}

		void _refill_tls_cache() {
			std::lock_guard<std::mutex> lk(global_lock_);
			auto& c = tls_cache_;
			const uint32_t want = TLSCacheSize;
			uint32_t got = 0;
			while (!free_list_.empty() && got < want) {
				c.buf[got++] = free_list_.back();
				free_list_.pop_back();
			}
			c.size = got;
			free_list_size_.store(free_list_.size(), std::memory_order_relaxed);
		}

		size_t _tls_total_cached() const noexcept {
			// Not exact across threads; just for rough stats
			return 0;
		}

		void _publish_alive_table_nolock(uint32_t cnt) {
			const uint32_t n = static_cast<uint32_t>(alive_blocks_.size()); // under lock
			// allocate exact n entries (can choose pow2 growth if you want fewer allocations)
			auto** new_tbl = static_cast<std::atomic<bool>**>(
				::operator new[](sizeof(std::atomic<bool>*) * cnt)
			);
			for (uint32_t i = 0; i < n; ++i) {
				new_tbl[i] = alive_blocks_[i].get();
			}

			// publish
			auto** old = alive_tbl_.load(std::memory_order_acquire);
			alive_tbl_.store(new_tbl, std::memory_order_release);
			alive_cnt_.store(n, std::memory_order_release);

			// retain old to free later (no hazard to readers)
			if (old) {
				alive_tbl_old_.push_back(old);
			}
		}

		void _publish_blocks_table(uint32_t cnt) {
			// build fresh table
			T** new_tbl = static_cast<T**>(::operator new[](sizeof(T*) * cnt));
			const size_t n = blocks_.size();
			for (uint32_t i = 0; i < n; ++i) {
				new_tbl[i] = blocks_[i];
			}
			// keep the rest undefined/unused

			// Publish (readers keep using old table until they next load)
			T** old = blocks_tbl_.load(std::memory_order_acquire);
			blocks_tbl_.store(new_tbl, std::memory_order_release);
			blocks_cnt_.store(n, std::memory_order_release);
			// Retain old table to free on destruction (no hazard to readers)
			if (old) {
				blocks_tbl_old_.push_back(old);
			}
		}

		void _free_tables() noexcept {
			for (auto* p : blocks_tbl_old_) {
				::operator delete[](p);
			}
			T** tbl = blocks_tbl_.load(std::memory_order_relaxed);
			if (tbl) {
				::operator delete[](tbl);
			}

			if (auto** p = alive_tbl_.load(std::memory_order_relaxed)) {
				::operator delete[](p);
				alive_tbl_.store(nullptr, std::memory_order_relaxed);
			}
			for (auto* p : alive_tbl_old_) {
				::operator delete[](p);
			}
			alive_tbl_old_.clear();
		}

		void _free_all_blocks() noexcept {
			for (T* b : blocks_) {
				::operator delete[](b, std::align_val_t(64));
			}
			blocks_.clear();
			alive_blocks_.clear();
			free_list_.clear();
			free_list_size_.store(0);

			for (size_t i = 0; i < TLSCacheSize; ++i) {
				tls_cache_.buf[i] = 0;
			}
			tls_cache_.size = 0;

			_free_tables();
		}
	
	private:
		std::mutex global_lock_;
		// -------- storage layout --------
		std::vector<T*> blocks_;                                         // slabs
		std::vector<std::unique_ptr<std::atomic<bool>[]>> alive_blocks_; // 1 byte each, ok for millions
		std::vector<uint32_t> free_list_;                                // global free ids (LIFO)
		std::atomic<size_t> free_list_size_ { 0 }; // approximate for stats	

		// -------- To remove race conditions while reallocating blocks need black magic with T** arrays --------
		// Published read-only table of block bases
		std::atomic<T**>                  blocks_tbl_{nullptr};
		std::atomic<uint32_t>             blocks_cnt_{0};
		
		std::atomic<std::atomic<bool>**> alive_tbl_{nullptr}; // array of pointers to per-block flags
		std::atomic<uint32_t>            alive_cnt_{0};       // number of published blocks

		// Keep all previous tables to free them in ~ObjectPool
		std::vector<std::atomic<bool>**> alive_tbl_old_;
		std::vector<T**> blocks_tbl_old_;

		// Per-thread cache of ids to avoid taking the global lock on every op.
		struct tls_cache_t {
			uint32_t buf[ObjectPool::TLSCacheSize];
			uint32_t size { 0 };
		};
		static thread_local tls_cache_t tls_cache_;
	};

	template<typename T, size_t BS, size_t CacheSize>
	thread_local typename ObjectPool<T, BS, CacheSize>::tls_cache_t ObjectPool<T, BS, CacheSize>::tls_cache_ {};


	template<typename _T, size_t _BlockSize = 65536u, size_t _TLSCacheSize = 1024u>
	class ObjectPoolExt : public ObjectPool<_T, _BlockSize, _TLSCacheSize> {
	public:
		using T = _T;
		static constexpr size_t BlockSize = _BlockSize;

		// Update internal objects, use locks so not use in parallel after acquire 
		void update_objects() const {
			if (_valid_objects.load(std::memory_order_acquire)) {
				return;
			}
			// no need to lock: read the snapshot table atomically
			auto** tbl = this->alive_table().load(std::memory_order_acquire);
			uint32_t n = this->alive_cnt().load(std::memory_order_acquire);

			_objects.clear();
			_objects.reserve(this->capacity());

			for (uint32_t b = 0; b < n; ++b) {
				auto* flags = tbl[b];
				for (uint32_t o = 0; o < BlockSize; ++o) {
					if (flags[o].load(std::memory_order_relaxed)) {
						uint32_t idx = b * BlockSize + o;
						_objects.push_back(this->_slot_ptr(idx));
					}
				}
			}

			_valid_objects.store(true, std::memory_order_release);
		}

		template<typename... Args>
		T* acquire_ptr(Args&&... args) {
			T* p = ObjectPool<_T, _BlockSize, _TLSCacheSize>::acquire(std::forward<Args>(args)...).ptr;
			_valid_objects.store(false, std::memory_order_release);
			return p;
		}

		void clear() {
			ObjectPool<_T, _BlockSize, _TLSCacheSize>::destroy_all_live();
			_objects.clear();
			_valid_objects.store(false, std::memory_order_release);
		}

		void release(T* ptr) {
			const uint32_t idx = this->_index_of_ptr(ptr);
			if (idx == UINT32_MAX) {
				return;
			}
			ObjectPool<_T, _BlockSize, _TLSCacheSize>::release({ptr, idx});
			_valid_objects.store(false, std::memory_order_release);
		}

		const std::vector<T*>& objects() const {
			update_objects();
			return _objects;
		}

	private:
		mutable std::atomic<bool> _valid_objects = true;
		mutable std::vector<T*> _objects;
	};

} // namespace tjs::common
