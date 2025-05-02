namespace tjs {
    class Application {
    public:
        void setFinished() {
            _isFinished = true;
        }

        bool isFinished() const {
            return _isFinished;
        }


    private:
        bool _isFinished = false;
    };
}