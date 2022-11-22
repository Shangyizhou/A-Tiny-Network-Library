#ifndef _CHRONO_TIMER_H__
#define _CHRONO_TIMER_H__

#include <chrono>
#include <string>

/**
 * start 开启定时器
 * reset 重启定时器
 * reference 记录基准时间
 * count 此函数内部会判断是定时器停止状态还是运行状态
 *  停止状态：直接返回累计时间，因此此时间会在 stop 函数内更新
 *  运行状态：需要计算当前时间 - 基准时间
*/
#include <chrono>


namespace cxxtimer {

/**
 * This class works as a stopwatch.
 */
class Timer {

public:

    /**
     * Constructor.
     *           
     * @param   start
     *          If true, the timer is started just after construction.
     *          Otherwise, it will not be automatically started.
     */
    Timer(bool start = false);

    /**
     * Copy constructor.
     *
     * @param   other
     *          The object to be copied.
     */
    Timer(const Timer& other) = default;

    /**
     * Transfer constructor.
     *
     * @param   other
     *          The object to be transferred.
     */
    Timer(Timer&& other) = default;

    /**
     * Destructor.
     */
    virtual ~Timer() = default;

    /**
     * Assignment operator by copy.
     *
     * @param   other
     *          The object to be copied.
     *
     * @return  A reference to this object.
     */
    Timer& operator=(const Timer& other) = default;

    /**
     * Assignment operator by transfer.
     *
     * @param   other
     *          The object to be transferred.
     *
     * @return  A reference to this object.
     */
    Timer& operator=(Timer&& other) = default;

    /**
     * Start/resume the timer.
     */
    void start();

    /**
     * Stop/pause the timer.
     */
    void stop();

    /**
     * Reset the timer.
     */
    void reset();

    /**
     * Return the elapsed time.
     *
     * @param   duration_t
     *          The duration type used to return the time elapsed. If not
     *          specified, it returns the time as represented by
     *          std::chrono::milliseconds.
     *
     * @return  The elapsed time.
     */
    template <class duration_t = std::chrono::milliseconds>
    typename duration_t::rep count() const;

private:

    bool started_;
    bool paused_;
    std::chrono::steady_clock::time_point reference_;
    std::chrono::duration<long double> accumulated_;
};

}


inline cxxtimer::Timer::Timer(bool start) :
        started_(false), paused_(false),
        reference_(std::chrono::steady_clock::now()),
        accumulated_(std::chrono::duration<long double>(0)) {
    if (start) {
        this->start();
    }
}

inline void cxxtimer::Timer::start() {
    if (!started_) {
        started_ = true;
        paused_ = false;
        accumulated_ = std::chrono::duration<long double>(0); // 累计时间
        reference_ = std::chrono::steady_clock::now(); // 基准时间
    } else if (paused_) {
        // 如果是 stop 之后的 timer，其会重新设置基准时间，但是之前的累计时间不会重置
        // 只有调用 reset 才会重置累计时间
        reference_ = std::chrono::steady_clock::now();
        paused_ = false;
    }
}

inline void cxxtimer::Timer::stop() {
    // 并未设置 started
    if (started_ && !paused_) {
        std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
        accumulated_ = accumulated_ + std::chrono::duration_cast< std::chrono::duration<long double> >(now - reference_); // 计算累计时间
        paused_ = true;
    }
}

inline void cxxtimer::Timer::reset() {
    if (started_) {
        started_ = false;
        paused_ = false;
        reference_ = std::chrono::steady_clock::now();
        accumulated_ = std::chrono::duration<long double>(0);
    }
}

// 可以是不同类型的比率
template <class duration_t>
typename duration_t::rep cxxtimer::Timer::count() const {
    if (started_) {
        if (paused_) {
            return std::chrono::duration_cast<duration_t>(accumulated_).count();
        } else {
            return std::chrono::duration_cast<duration_t>(
                    accumulated_ + (std::chrono::steady_clock::now() - reference_)).count();
        }
    } else {
        return duration_t(0).count();
    }
}

#endif // _CHRONO_TIMER_H__