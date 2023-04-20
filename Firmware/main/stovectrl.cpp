#include "stovectrl.h"

#include "cooktimers.h"

#include "mcp9600.h"
#include "httpd.h"
#include "led.h"

#include <mutex>

constexpr const static gpio_num_t Downdraft_Low  = gpio_num_t(42);
constexpr const static gpio_num_t Downdraft_High = gpio_num_t(41);
constexpr const static gpio_num_t Cooling_Fan    = gpio_num_t(40);
constexpr const static gpio_num_t Fan_Low        = gpio_num_t(37);
constexpr const static gpio_num_t Fan_High       = gpio_num_t(36);

constexpr const static gpio_num_t Light          = gpio_num_t(38);

constexpr const static gpio_num_t Bake_A         = gpio_num_t(35);
constexpr const static gpio_num_t Bake_B         = gpio_num_t(34);
constexpr const static gpio_num_t Broil_A        = gpio_num_t(33);
constexpr const static gpio_num_t Broil_B        = gpio_num_t(26);
constexpr const static gpio_num_t Convection_A   = gpio_num_t(21);

constexpr const static gpio_num_t Door_Open      = gpio_num_t( 4);
constexpr const static gpio_num_t Door_Locked    = gpio_num_t( 3);
constexpr const static gpio_num_t Door_Unlocked  = gpio_num_t( 2);
constexpr const static gpio_num_t Cancel         = gpio_num_t( 1);

static std::string to_string(StoveCtrlMode mode)
{
    switch (mode)
    {
    case SCM_Off: return "off";
    case SCM_Manual: return "low";
    case SCM_Timers: return "high";
    }
    return "";
};

static std::string to_string(FanSpeed speed)
{
    switch (speed)
    {
    case FS_Off: return "off";
    case FS_Low: return "low";
    case FS_High: return "high";
    }
    return "";
}

class ElementCtrl
{

    /* When doing a temperature rise, run the element for an amount of time.
     *   After that time has elapsed, let the element cool and dissipate the heat into the air.
     * We could try to calculate this increase, but it'd probabbly be wrong.... so we determine it experimentally.
     * <Seconds Active>  <Delta T>
     *
     *
     */

    enum Heating_State
    {
        Dormant,
        Heating,
        WaitingForEffect,
    } m_current_heating_state { Dormant };

    int m_heating_state_dwell_time_s { 0 };

    bool top_burner_active { false };
    bool bot_burner_active { false };

    bool top_burner_used { false };
    bool bot_burner_used { false };

public:
    void off()
    {
        gpio_set_level(Bake_A,          false);
        gpio_set_level(Bake_B,          false);
        gpio_set_level(Broil_A,         false);
        gpio_set_level(Broil_B,         false);
        gpio_set_level(Convection_A,    false);
    }

    // Thermal power can be used to estimate the needed heat rise to guess how long
    //    to keep the element(s) on given the current temperature.
    static constexpr int top_element_power_w = 1100;
    static constexpr int bot_element_power_w = 1500;
    static constexpr int q_loss = 1; // heat flow lost from box due to imperfect insulation

    /* From https://www.engineeringtoolbox.com/convective-heat-transfer-d_430.html
     * Here we can define Newton's Law of Cooling:
     *      q = hc A dT
     * where
     *      q = heat transferred per unit time (W, Btu/hr)
     *      A = heat transfer area of the surface (m2, ft2)
     *     hc = convective heat transfer coefficient of the process (W/(m2oC, Btu/(ft2 h oF))
     *     dT = temperature difference between the surface and the bulk fluid (oC, F)
     *
     * From this we see that the difference in temperature of the element and the air defines a
     *   linear increase in the heat transferred into the air per unit time.
     *
     * We can also reason that the amount of time for the element to reach a given temperature
     *   is dictated by the starting temperature of the element. This would be defined by the
     *   heat loss to the external system, and the transfer to/from the oven air.
     */


    void update()
    {

    }

    void use_top_burner(bool state) { top_burner_used = state; }
    void use_bot_burner(bool state) { bot_burner_used = state; }
    bool use_top_burner() const { return top_burner_used; }
    bool use_bot_burner() const { return bot_burner_used; }
};

class StoveCtrl
{
    led_strip_t *m_led;

    ElementCtrl m_elementCtrl;

    static constexpr float m_auto_cool_temp { 150.0 };

    float m_current_temp { 0.0 };
    float m_target_temp { 0.0 };

    StoveCtrlMode m_mode { SCM_Off };
    StoveCtrlMode m_prev_mode { SCM_Off };

    FanSpeed m_downdraft_fan_speed { FS_Off };
    FanSpeed m_convection_fan_speed { FS_Off };
    bool m_cooling_fan_state { false };

    bool m_light_state { false };

    int m_cancel_count { 0 };

    void state_off()
    {
        m_target_temp = 0;

        if (m_prev_mode != m_mode)
            setConvectionFan(FS_Off);

        m_elementCtrl.off();

        // When off we want to just cool down.
        if (m_cooling_fan_state)
            m_cooling_fan_state = m_current_temp > m_auto_cool_temp;
        setCoolingFan(m_cooling_fan_state);
    }

    // Probabbly just going to be a NOP
    void state_manual()
    {

    }

    // Check if the current timer is active, if not remove it and activate the next one.
    void state_timers()
    {
        CT_update();
    }

    void check_cancel()
    {
        if(!gpio_get_level(Cancel))
        {
            m_cancel_count = 0;
        }
        else if (m_cancel_count < 50)
        {
            m_cancel_count++;
        }
        else
        {
            cancel();
        }
    }

    void update_temp()
    {
        m_current_temp = mcp9600_get_temp_F();
        if (m_current_temp > 800)
        {
            cancel();
            printf("Run away or error state!\n");
        }
    }

public:
    explicit StoveCtrl(led_strip_t *led) : m_led(led)
    {
        state_off();
        setDowndraftFan(FS_Off);
        setLight(false);

        led->set_pixel(led, 0, 0x255, 0x255, 0x255);
        led->refresh(led, 100);
    }

    void setTargetTemp(float target_temp)
    {
        m_target_temp = target_temp;
    }

    void setMode(StoveCtrlMode mode)
    {
        m_mode = mode;
    }

    void setDowndraftFan(FanSpeed level)
    {
        m_downdraft_fan_speed = level;
        gpio_set_level(Downdraft_Low,  level == FS_Low);
        gpio_set_level(Downdraft_High, level == FS_High);
    }

    void setConvectionFan(FanSpeed level)
    {
        m_convection_fan_speed = level;
        gpio_set_level(Fan_Low,  level == FS_Low);
        gpio_set_level(Fan_High, level == FS_High);
    }

    void setCoolingFan(bool state)
    {
        m_cooling_fan_state = state;
        gpio_set_level(Cooling_Fan,  state);
    }

    void setLight(bool light)
    {
        m_light_state = light;
        gpio_set_level(Light, light);
    }

    void setUseBottomElement(bool state)
    {
        m_elementCtrl.use_bot_burner(state);
    }

    void setUseTopElement(bool state)
    {
        m_elementCtrl.use_top_burner(state);
    }

    void setStoveMode(StoveCtrlMode mode)
    {
        m_mode = mode;
    }

    bool isDoorOpen()
    {
        return gpio_get_level(Door_Open);
    }

    void cancel()
    {
        state_off();
        m_mode = SCM_Off;
    }

    void update()
    {
        update_temp();

        switch (m_mode)
        {
        case SCM_Off:
            state_off();
            return;

        case SCM_Manual:
            state_manual();
            break;

        case SCM_Timers:
            state_timers();
            break;
        }

        check_cancel();

        // previous commands could have errored out.
        // If so they are required to have called cancel() for us!
        if (m_mode == SCM_Off)
        {
            state_off();
        }
        else
        {
            m_elementCtrl.update();
        }

        m_prev_mode = m_mode;
    }

    void toJSON(std::string &buf)
    {
        buf += "\"current_temp\":"     + std::to_string(m_current_temp) + ","
               "\"target_temp\":"      + std::to_string(m_target_temp) + ","
               "\"stove_mode\":\""     + to_string(m_mode) + "\","
               "\"downdraft_fan\":\""  + to_string(m_downdraft_fan_speed) + "\","
               "\"convection_fan\":\"" + to_string(m_convection_fan_speed) + "\","
               "\"cooling_fan\":"      + std::to_string(m_cooling_fan_state) + ","
               "\"light\":"            + std::to_string(m_light_state) + ","
               "\"use_top_burner\":"   + std::to_string(m_elementCtrl.use_top_burner()) + ","
               "\"use_bot_burner\":"   + std::to_string(m_elementCtrl.use_bot_burner());
    }
};

static StoveCtrl *SC = nullptr;

void SC_init(led_strip_t *led)
{
    SC = new StoveCtrl(led);
}


// None of these functions may run while update() is running
static std::mutex mutex;

void SC_set_target_temp(double target_temp)
{
    // No Mutex. Only called by cook timer from SC->update()
    SC->setTargetTemp(target_temp);
}

void SC_task_event()
{
    const std::lock_guard<std::mutex> lock(mutex);
    SC->update();
}

void SC_init_gpio()
{
    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_DISABLE; // disable interrupt
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask =
            (1ULL << Downdraft_Low)  |
            (1ULL << Downdraft_High) |
            (1ULL << Cooling_Fan)    |
            (1ULL << Light)          |
            (1ULL << Fan_Low)        |
            (1ULL << Fan_High)       |
            (1ULL << Bake_A)         |
            (1ULL << Bake_B)         |
            (1ULL << Broil_A)        |
            (1ULL << Broil_B)        |
            (1ULL << Convection_A);

    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;

    gpio_config(&io_conf);
}

esp_err_t get_state(httpd_req_t *req)
{
    const std::lock_guard<std::mutex> lock(mutex);

    std::string json;
    json.reserve(1024);

    json = "{ \"state\": {"; // space incase timers is empty.
    SC->toJSON(json);
    json += "}}";

    httpd_resp_set_type(req, "application/json");
    return httpd_resp_send(req, json.c_str(), json.size());
}

esp_err_t set_target_temperature(httpd_req_t *req)
{
    char content[100];
    if (ESP_OK != get_content(req, content, sizeof(content)))
        return ESP_FAIL;

    double new_temp = std::stod(content);
    if ((new_temp < 500) && (new_temp >= 0))
    {
        const std::lock_guard<std::mutex> lock(mutex);
        SC->setTargetTemp(new_temp);
    }

    printf("Set Target!\n");
    printf(content);
    printf("\n");

    return ack_http_post(req);
}

esp_err_t set_light(httpd_req_t *req)
{
    char content[100];
    if (ESP_OK != get_content(req, content, sizeof(content)))
        return ESP_FAIL;

    {
        const std::lock_guard<std::mutex> lock(mutex);
        SC->setLight(std::string(content) == "1");
    }

    printf("Set Light!\n");
    printf(content);
    printf("\n");

    return ack_http_post(req);
}

esp_err_t set_cooling_fan(httpd_req_t *req)
{
    char content[100];
    if (ESP_OK != get_content(req, content, sizeof(content)))
        return ESP_FAIL;

    {
        const std::lock_guard<std::mutex> lock(mutex);
        SC->setCoolingFan(std::string(content) == "1");
    }

    // SC_set_temp
    printf("Set Cooling Fan!\n");
    printf(content);
    printf("\n");

    return ack_http_post(req);
}

esp_err_t set_downdraft_fan(httpd_req_t *req)
{
    char content[100];
    if (ESP_OK != get_content(req, content, sizeof(content)))
        return ESP_FAIL;

    {
        const std::string_view str(content);
        const std::lock_guard<std::mutex> lock(mutex);

        if (str == "0")
        {
            SC->setDowndraftFan(FS_Off);
        }
        else if (str == "1")
        {
            SC->setDowndraftFan(FS_Low);
        }
        else if (str == "2")
        {
            SC->setDowndraftFan(FS_High);
        }
    }

    // SC_set_temp
    printf("Set Downdraft Fan!\n");
    printf(content);
    printf("\n");

    return ack_http_post(req);
}

esp_err_t set_convection_fan(httpd_req_t *req)
{
    char content[100];
    if (ESP_OK != get_content(req, content, sizeof(content)))
        return ESP_FAIL;

    {
        const std::string_view str(content);
        const std::lock_guard<std::mutex> lock(mutex);

        if (str == "0")
        {
            SC->setConvectionFan(FS_Off);
        }
        else if (str == "1")
        {
            SC->setConvectionFan(FS_Low);
        }
        else if (str == "2")
        {
            SC->setConvectionFan(FS_High);
        }
    }

    // SC_set_temp
    printf("Set Convection Fan!\n");
    printf(content);
    printf("\n");

    return ack_http_post(req);
}

esp_err_t set_stove_mode(httpd_req_t *req)
{
    char content[100];
    if (ESP_OK != get_content(req, content, sizeof(content)))
        return ESP_FAIL;

    {
        const std::string_view str(content);
        const std::lock_guard<std::mutex> lock(mutex);

        if (str == "0")
        {
            SC->setStoveMode(SCM_Off);
        }
        else if (str == "1")
        {
            SC->setStoveMode(SCM_Manual);
        }
        else if (str == "2")
        {
            SC->setStoveMode(SCM_Timers);
        }
    }

    // SC_set_temp
    printf("Set Use Top Element!\n");
    printf(content);
    printf("\n");

    return ack_http_post(req);
}

esp_err_t set_use_top_element(httpd_req_t *req)
{
    char content[100];
    if (ESP_OK != get_content(req, content, sizeof(content)))
        return ESP_FAIL;

    {
        const std::lock_guard<std::mutex> lock(mutex);
        SC->setUseTopElement(std::string(content) == "1");
    }

    // SC_set_temp
    printf("Set Use Top Element!\n");
    printf(content);
    printf("\n");

    return ack_http_post(req);
}

esp_err_t set_use_bottom_element(httpd_req_t *req)
{
    char content[100];
    if (ESP_OK != get_content(req, content, sizeof(content)))
        return ESP_FAIL;

    {
        const std::lock_guard<std::mutex> lock(mutex);
        SC->setUseBottomElement(std::string(content) == "1");
    }

    // SC_set_temp
    printf("Set Use Bottom Element!\n");
    printf(content);
    printf("\n");

    return ack_http_post(req);
}
