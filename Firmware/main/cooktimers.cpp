#include "cooktimers.h"
#include "stovectrl.h"

#include <mutex>
#include <chrono>
#include <string>
#include <vector>
#include <functional>

#include "httpd.h"

using namespace std::chrono_literals;

// https://codereview.stackexchange.com/a/225927
class ElapsedTimer
{
protected:
    using clock = std::chrono::steady_clock;

private:
    clock::duration m_elapsedTime = {0s};
    clock::time_point m_startTime = {};
    static std::mutex timer_mutex;

    bool isRun() const
    {
        return m_startTime != clock::time_point{};
    }

public:
    bool isRunning() const
    {
        const std::lock_guard<std::mutex> lock(timer_mutex);
        return isRun();
    }

    void start()
    {
        const std::lock_guard<std::mutex> lock(timer_mutex);
        if (!isRun())
        {
            m_startTime = clock::now();
        }
    }

    void pause()
    {
        const std::lock_guard<std::mutex> lock(timer_mutex);
        if (isRun())
        {
            m_elapsedTime += clock::now() - m_startTime;
            m_startTime = {};
        }
    }

    void reset()
    {
        const std::lock_guard<std::mutex> lock(timer_mutex);
        m_elapsedTime = clock::duration{0s};
        m_startTime = {};
    }

    clock::duration elapsedTime() const
    {
        const std::lock_guard<std::mutex> lock(timer_mutex);
        clock::duration result = m_elapsedTime;
        if (isRun())
        {
            result += clock::now() - m_startTime;
        }
        return result;
    }
};

std::mutex ElapsedTimer::timer_mutex;

class Timer : public ElapsedTimer
{
    clock::duration m_interval = {};
public:
    explicit Timer(clock::duration interval)
        : m_interval(interval)
    {

    }

    clock::duration duration() const
    {
        return m_interval;
    }

    bool timedout() const
    {
        return m_interval < elapsedTime();
    }
};

class CookTimer
{

public:
    enum Action
    {
        Cook,
        Stop_Cook,
        Stop_n_Cool,
        Countdown,
        Alert_Webpage
    };

    explicit CookTimer(const CookTimer &t)
        : uid(t.uid)
        , argument(t.argument)
        , action(t.action)
        , timer(t.timer)
    {

    }

    CookTimer &operator=(const CookTimer &&timer)
    {
        *this = CookTimer(timer);
        return *this;
    }

    explicit CookTimer(Action action, int time_s, double argument)
        : uid(next_uid())
        , argument(argument)
        , action(action)
        , timer(std::chrono::seconds(time_s))
    {

    }

    void start()
    {
        start_action();
        timer.start();
    }

    void pause()
    {
        timer.pause();
    }

    bool isRunning() const
    {
        return timer.isRunning();
    }

    bool done() const
    {
        return timer.isRunning() && timer.timedout();
    }

    void toJSON(std::string &buf) const
    {
        using namespace std::chrono;
        const auto time = duration_cast<seconds>(timer.elapsedTime());
        const auto duration = duration_cast<seconds>(timer.duration());

        buf += "\n\t{"
               "\n\t\t\"uid\":" + std::to_string(uid)
            +  ",\n\t\t\"elapsed\": " + std::to_string(time.count())
            +  ",\n\t\t\"duration\": " + std::to_string(duration.count())
            +  ",\n\t\t\"argument\": \"" + argument_string() +  "\""
            +  ",\n\t\t\"action\": \"" + to_string(action) + "\""
               "\n\t}";
    }

    void check()
    {
        if (done())
        {
            done_action();
        }
    }

    int id() const
    {
        return uid;
    }

    static Action from_post_string(const std::string &a)
    {
        if (a == "Cook") return Cook;
        if (a == "Countdown") return Countdown;
        if (a == "Stop+&+Cool") return Stop_n_Cool;
        if (a == "Alert+Webpage") return Alert_Webpage;

        //if (a == "Stop+Cook") return Stop_Cook;
        return Stop_Cook;
    }

private:
    static std::mutex mutex;
    const int uid;
    const double argument;
    const Action action { Countdown };
    Timer timer;

    static int next_uid()
    {
        static int i = 0;
        return ++i;
    }

    static std::string to_string(Action a)
    {
        switch(a)
        {
        case Cook:          return "Cook";
        case Countdown:     return "Countdown";
        case Stop_Cook:     return "Stop Cook";
        case Stop_n_Cool:   return "Stop & Cool";
        case Alert_Webpage: return "Alert Webpage";
        }
        return "";
    }

    std::string argument_string() const
    {
        if(action != Cook) return "";
        return std::to_string(argument);
    }

    void start_action()
    {
        printf("Timer %i started!\n", uid);

        switch(action)
        {
        case Cook:
            SC_set_target_temp(argument);
            return;

        case Countdown:
        case Stop_Cook:
        case Stop_n_Cool:
        case Alert_Webpage:
            return;
        }

    }

    void done_action()
    {
        printf("Timer %i timed out!\n", uid);

        switch(action)
        {
        case Cook:
        case Countdown:
        case Alert_Webpage:
            return;

        case Stop_Cook:
            SC_set_target_temp(0);
            return;
        case Stop_n_Cool:
            SC_set_target_temp(0);
            return;
        }

    }
};

std::mutex CookTimer::mutex;

/*******************************/

#define MAX_TIMERS 30
static std::vector<CookTimer> timers;
static std::mutex timers_mutex;

void CT_task_init()
{
    const std::lock_guard<std::mutex> lock(timers_mutex);
    timers.reserve(MAX_TIMERS);
}

void CT_update()
{
    const std::lock_guard<std::mutex> lock(timers_mutex);

    if (timers.empty())
        return;

    if (timers.at(0).isRunning())
    {
        timers.at(0).check();
    }
    else
    {
        timers.at(0).start();
    }

    if(timers.at(0).done())
    {
        timers.erase(timers.begin());
    }
}

/*******************************/

esp_err_t get_timers(httpd_req_t *req)
{
    const std::lock_guard<std::mutex> lock(timers_mutex);

    std::string json;
    json.reserve(1024);

    json = "{ \"timers\" : [ "; // space incase timers is empty.
    for (size_t i = 0; i < timers.size(); i++)
    {
        timers.at(i).toJSON(json);

        json += ",";
    }
    json[json.size()-1] = ']';
    json += '}';

    httpd_resp_set_type(req, "application/json");

    return httpd_resp_send(req, json.c_str(), json.size());
}

std::vector<std::string> split(const std::string &str, char delim1 = '&', char delim2 = '=')
{
    std::vector<std::string> res;

    size_t last = std::min(str.find(delim1, 0),
                           str.find(delim2, 0));
    if (last != std::string::npos)
        res.push_back(str.substr(0, last));

    while (last != std::string::npos)
    {
        const size_t next = std::min(str.find(delim1, last+1),
                                     str.find(delim2, last+1));

        if (next != std::string::npos)
            res.push_back(str.substr(last+1, next-last-1));
        else // grab the end of the array
            res.push_back(str.substr(last+1));

        last = next;
    }

    return res;
}


esp_err_t add_timer(httpd_req_t *req)
{
    char content[100];
    if (ESP_OK != get_content(req, content, sizeof(content)))
        return ESP_FAIL;

    //duration=36000&argument=100&action=Set+Temperature
    auto s = split(content);

    if (s.size() != 6)
    {
        printf("Incorrect number of arguments for timer!\n");
    }
    else if(s.at(0) != "duration")
    {
        printf("Incorrect order" "(1)\n");
    }
    else if(s.at(2) != "argument")
    {
        printf("Incorrect order" "(2)\n");
    }
    else if(s.at(4) != "action")
    {
        printf("Incorrect order" "(3)\n");
    }
    else
    {
        for(auto i : s) {
            printf(i.c_str());
            printf("\n");
        }

        const int duration = std::stoi(s.at(1));
        const double argument = std::stod(s.at(3));
        const auto action = CookTimer::from_post_string(s.at(5));

        const std::lock_guard<std::mutex> lock(timers_mutex);
        timers.push_back(CookTimer(action, duration, argument));
    }

    printf("Add Timer\n");
    printf(content);
    printf("\n");

    return ack_http_post(req);
}

esp_err_t rm_timer(httpd_req_t *req)
{
    char content[100];
    if (ESP_OK != get_content(req, content, sizeof(content)))
        return ESP_FAIL;

    const int uid = std::atoi(content);

    {
        const std::lock_guard<std::mutex> lock(timers_mutex);
        for (size_t i = 0; i < timers.size(); i++)
        {
            if (timers.at(i).id() == uid)
            {
                timers.erase(timers.begin() + i);
                break;
            }
        }
    }

    printf("Remove Timer %i!\n", uid);
    printf(content);
    printf("\n");

    return ack_http_post(req);
}
