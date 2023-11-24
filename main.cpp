#include <cassert>
#include <functional>
#include <memory>
#include <optional>
#include <thread>
#include <variant>
#include <vector>

#include "util.hpp"

namespace neszenz {

class ITask {
public:
    virtual ~ITask() = default;

    virtual bool done() const = 0;

    virtual SteadyTimepoint idleUntil() const = 0;

    virtual void resume() = 0;
};

template <typename T>
class Task final : public ITask {
public:
    struct RepeatStage { Duration delay = Duration{0}; };
    struct NextStage { Duration delay = Duration{0}; };
    struct AwaitTask { std::shared_ptr<ITask> task; };
    struct Result { T value; };
    using ReturnValue = std::variant<RepeatStage, NextStage, AwaitTask, Result>;

    using StageFn = std::function<ReturnValue(void)>;

    Task(const std::vector<StageFn> stages) : stages_{stages} {}

    bool done() const
    {
        return stage_counter_ >= stages_.size();
    }

    SteadyTimepoint idleUntil() const
    {
        return idle_until_;
    }

    std::optional<T> result() const
    {
        if (auto e = this->unhandled_exception_) {
            std::rethrow_exception(*e);
        }
        else if (auto v = this->result_value_) {
            return *v;
        }
        else {
            return {};
        }
    }

    void resume()
    {
        if (this->done() || SteadyClock::now() < this->idle_until_) {
            return;
        }

        if (nested_task != nullptr) {
            nested_task->resume();

            if (nested_task->done()) {
                nested_task.reset();
            }
            else {
                idle_until_ = nested_task->idleUntil();
            }
        }

        if (nested_task == nullptr) {
            try {
                ReturnValue ret = stages_.at(stage_counter_)();

                if (auto* rs = std::get_if<RepeatStage>(&ret)) {
                    idle_until_ = SteadyClock::now() + rs->delay;
                }
                else if (auto* ns = std::get_if<NextStage>(&ret)) {
                    stage_counter_ += 1;
                    idle_until_ = SteadyClock::now() + ns->delay;
                }
                else if (auto* a = std::get_if<AwaitTask>(&ret)) {
                    stage_counter_ += 1;
                    nested_task = a->task;
                }
                else if (auto* r = std::get_if<Result>(&ret)) {
                    stage_counter_ += 1;
                    result_value_ = r->value;
                }
                else {
                    UNIMPLEMENTED;
                }
            }
            catch (...) {
                stage_counter_ = stages_.size();
                unhandled_exception_ = std::current_exception();
            }
        }
    }

    void exhaust()
    {
        println("Task::exhaust");
        while (not this->done()) {
            std::this_thread::sleep_until(idle_until_);
            this->resume();
        }
    }

private:
    std::vector<StageFn> stages_;
    size_t stage_counter_ = 0;

    SteadyTimepoint idle_until_ = SteadyClock::now();
    std::shared_ptr<ITask> nested_task = nullptr;

    std::optional<std::exception_ptr> unhandled_exception_ = {};
    std::optional<T> result_value_ = {};
};

template <typename TYPE>
struct TaskSpecialization {
    using T = TYPE;
    using Result = Task<T>::Result;
    using RepeatStage = Task<T>::RepeatStage;
    using NextStage = Task<T>::NextStage;
    using AwaitTask = Task<T>::AwaitTask;
    struct Arguments { /* empty */ };
    struct RuntimeState { /* empty */ };
};

#define TASK(NAME, TYPE) struct NAME : TaskSpecialization<TYPE>

#define STAGES(...)                                                                                                   \
    inline static Task<T> build(const Arguments args)                                                                  \
    {                                                                                                                  \
        auto rts = std::make_shared<RuntimeState>();                                                                   \
        return Task<T>{std::vector<Task<T>::StageFn>{__VA_ARGS__}};                                                           \
    }

#define STAGE [args, rts]() -> Task<T>::ReturnValue

#define BUILD_TASK(TASK_NAME, ...) TASK_NAME::build({__VA_ARGS__})

TASK(Bar, int) {
    struct Arguments {
        Duration d;
    };

    STAGES (
        STAGE {
            return NextStage{args.d};
        },
        STAGE {
            return Result{42};
        }
    )
};

TASK(Foo, float) {
    struct RuntimeState {
        std::shared_ptr<Task<int>> bar;
    };

    STAGES(
        STAGE {
            rts->bar = std::make_shared<Task<int>>(BUILD_TASK(Bar, Seconds{1}));
            return AwaitTask{rts->bar};
        },
        STAGE {
            assert(rts->bar->done());
            return Result{0.1f * rts->bar->result().value()};
        }
    )
};

} // namespace neszenz

int main(void)
{
    using namespace neszenz;

    int i = 0;
    println("i={}", i);

    auto t = BUILD_TASK(Foo);

    const auto tp = SteadyClock::now();
    t.exhaust();
    println("exhaused after {}", fSeconds{SteadyClock::now() - tp});

    assert(t.done());
    assert(t.result().has_value());
    println("result={}", fSeconds{*t.result()});
    println("i={}", i);

    return 0;
}
