const max_allowed_temp = 500;

// On resize we find the size of the sliders and adjust the animation to fit to the screen
function resize_window()
{
   var width = $('#light-span').width() - 34;

   appendStyle(
       ".switch input:checked + .switch.slider:before {" +
       " -webkit-transform: translateX(" + width + "px);" +
       " -ms-transform: translateX(" + width + "px);" +
       "  transform: translateX(" + width + "px); }");
}

function set_target_temperature()
{
    var value = document.getElementById("target_temp-input").value;

    if (value == null || value == "") return;

    if (value >= max_allowed_temp)
    {
        document.getElementById("target_temp-input").value = "";
        return;
    }

    console.log("Setting temperature to: " + value + "F");

    $.ajax({
        type:'post',
        url:'set_target_temp',
        data: value
    });
}

function set_use_top_element(cb)
{
    if (cb == null) return;

    var value = cb.checked ? '1' : '0';
    console.log("Setting Use Top Element to: " + value);

    $.ajax({
        type:'post',
        url:'set_use_top_element',
        data: value,
    });
}

function set_use_bottom_element(cb)
{
    if (cb == null) return;

    var value = cb.checked ? '1' : '0';
    console.log("Setting Use Bottom Element to: " + value);

    $.ajax({
        type:'post',
        url:'set_use_bottom_element',
        data: value,
    });
}

function set_light(cb)
{
    if (cb == null) return;

    var value = cb.checked ? '1' : '0';
    console.log("Setting Light to: " + value);

    $.ajax({
        type:'post',
        url:'set_light',
        data: value,
    });
}

function set_cooling_fan(cb)
{
    if (cb == null) return;

    var value = cb.checked ? '1' : '0';
    console.log("Setting Cooling Fan to: " + value);

    $.ajax({
        type:'post',
        url:'set_cooling_fan',
        data: value
    });
}

function set_downdraft_fan(value)
{
    if (value >= 0)
    {
        console.log("Setting Downdraft Fan to: " + value);

        $.ajax({
            type:'post',
            url:'set_downdraft_fan',
            data: value
        });
    }
}

function set_convection_fan(value)
{
    if (value >= 0)
    {
        console.log("Setting Convection Fan to: " + value);

        $.ajax({
            type:'post',
            url:'set_convection_fan',
            data: value
        });
    }
}

function set_stove_mode(value)
{
    if (value >= 0)
    {
        console.log("Setting Stove Mode to: " + value);

        $.ajax({
            type:'post',
            url:'set_stove_mode',
            data: value
        });
    }
}

function appendStyle(styles)
{
    var css = document.createElement('style');
    css.type = 'text/css';

    if (css.styleSheet)
    {
        css.styleSheet.cssText = styles;
    }
    else
    {
        css.appendChild(document.createTextNode(styles));
    }

    document.getElementsByTagName("head")[0].appendChild(css);
}

function create_select(current_text)
{
    var sel = document.createElement('SELECT');

    // Must match the server code to be accepted
    const texts = [ "Cook", "Stop Cook", "Stop & Cool", "Countdown", "Alert Webpage" ];
    for (const text of texts)
    {
        opt = document.createElement('OPTION');
        opt.value = text;
        opt.innerHTML = text;
        sel.appendChild(opt);
    }

    sel.value = current_text;

    return sel;
}

function create_remove_button(uid)
{
    var btn = document.createElement('BUTTON');
    btn.className = "remove_timer";
    btn.id = uid + "_remove_btn";

    btn.addEventListener("click", function(){remove_timer(uid);});

    return btn;
}

function to_time(seconds)
{
    return new Date(seconds * 1000).toISOString().slice(11, 19);
}

function add_timer(timer)
{
    var tr = document.createElement('TR');
    tr.id = timer.uid + "_uid";

    var td = [];
    var ids = [ "elapsed", "duration", "argument", "action", "remove" ];
    for(var i = 0; i < 5; i++)
    {
        t = document.createElement('TD')
        t.id = timer.uid + "_" + ids[i];
        td.push(t);
    }

    var argument_val = timer.argument;
    if (argument_val == undefined)
        argument_val = "";

    td[0].appendChild(document.createTextNode(to_time(timer.duration - timer.elapsed)));
    td[1].appendChild(document.createTextNode(to_time(timer.duration)));
    td[2].appendChild(document.createTextNode(argument_val));
    td[3].appendChild(document.createTextNode(timer.action));
    td[4].appendChild(create_remove_button(timer.uid));

    for(const t of td)
        tr.appendChild(t);

    return tr;
}

function update_timer(timer)
{
    var elapsed  = document.getElementById(timer.uid + "_elapsed");
    var duration = document.getElementById(timer.uid + "_duration");
    var action   = document.getElementById(timer.uid + "_action");
    var argument = document.getElementById(timer.uid + "_argument");

    var argument_val = timer.argument;
    if (argument_val == undefined)
        argument_val = "";

    elapsed.innerHTML = to_time(timer.duration - timer.elapsed);
    duration.innerHTML = to_time(timer.duration);
    argument.innerHTML = argument_val;
    action.value = timer.action;

    // TODO: show timer < 10s with red background.
}

// Loop through the table and the timers and remove the dead timers from the table
function remove_dead_timers(table, timers)
{
    var uid_list = [];
    for (const timer of timers)
    {
        uid_list.push(timer.uid + "_uid");
    }

    var purge_list = [];
    for (const tableChild of table.children)
    {
        // Skip our special rows
        if (!tableChild.id.endsWith("_uid"))
            continue;

        if (!uid_list.includes(tableChild.id))
            purge_list.push(tableChild.id);
    }

    for (const purge of purge_list)
    {
        const element = document.getElementById(purge);
        element.remove();
    }
}

function remove_timer(uid)
{
    console.log("Remove Timer " + uid);

    $.ajax({
        type:'post',
        url:'rm_timer',
        data: String(uid)
    });
}

function show_timers(timers)
{
    var table = document.getElementById("timers_table");

    for(const timer of timers)
    {
        if (document.getElementById(timer.uid + "_uid") != undefined)
        {
            update_timer(timer);
        }
        else
        {
            tr = add_timer(timer);
            table.appendChild(tr);
        }
    }

    remove_dead_timers(table, timers);
}

function make_timer()
{
    var new_duration = document.getElementById("new_task_hour").value;
    var new_action   = document.getElementById("new_timer_action").value;
    var new_argument = document.getElementById("new_task_argument").value;

    if ((new_action == "") || (new_duration == ""))
    {
        return;
    }

    if (new_duration > 24)
    {
        console.log("You requested to cook something for more than 24 hours. I won't do that.");
        return;
    }

    if (new_action == "Set Temperature")
    {
        if (new_argument == "")
        {
            // We could be nice and set the box red now
            console.log("You forgot to give the temperature");
            return;
        }

        if (new_argument >= max_allowed_temp)
        {
            console.log("You set the temperatures /very/ high. I won't do that.");
            return;
        }
    }

    var data = {
        duration: new_duration * 3600,
        argument: new_argument,
        action:   new_action
    };

    $.ajax({
        type:'post',
        url:'add_timer',
        data: data
    });
}

function set_input_state(state)
{
    $("#downdraft-off").removeClass("active_off");
    $("#downdraft-low").removeClass("active_low");
    $("#downdraft-high").removeClass("active_high");
    $("#downdraft-" + state.downdraft_fan).addClass("active_" + state.downdraft_fan);

    $("#convection_fan-off").removeClass("active_off");
    $("#convection_fan-low").removeClass("active_low");
    $("#convection_fan-high").removeClass("active_high");
    $("#convection_fan-" + state.convection_fan).addClass("active_" + state.convection_fan);

    $("#stove_mode-off").removeClass("active_off");
    $("#stove_mode-low").removeClass("active_low");
    $("#stove_mode-high").removeClass("active_high");
    $("#stove_mode-" + state.stove_mode).addClass("active_" + state.stove_mode);

    document.getElementById("light-input").checked = state.light;
    document.getElementById("cooling_fan-input").checked = state.cooling_fan;
    document.getElementById("use_bot_elem-input").checked = state.use_bot_burner;
    document.getElementById("use_top_element-input").checked = state.use_top_burner;
}

function set_temp_state(state)
{
    document.getElementById("act_current_temp").innerHTML = parseFloat(state.current_temp).toFixed(2) + "&#8457;";
    document.getElementById("act_target_temp").innerHTML = state.target_temp + "&#8457;";
}

// TODO: Some day we should come up with a way to make all previous get_state calls be ignored till
//         after we get the first state from after the user interacts with the interface.
update_counter = 30 // start with a large enough value to force setup
function set_state(state)
{
    update_counter++;
    if(update_counter > 30) // Only update GUI elements ever 30s
    {
      update_count = 0;
      set_input_state(state);
    }

    set_temp_state(state);
}

// Schedule getting the cook timers
function get_timers()
{
    var call = function() { $.ajax({
        type:'get',
        url:'get_timers.json',
        success: function(data)
        {
            timers = data.timers;
            show_timers(timers);
        }
    })};

    setInterval(call, 1000);
    call();
}

// Schedule getting the oven status
function get_state()
{
    var call = function() { $.ajax({
        type:'get',
        url:'get_state.json',
        success: function(data)
        {
            set_state(data.state);
        }
    })};

    setInterval(call, 1000);
    call();
}


// READY! SET! GO!
$(document).ready(function()
{
    $(".tri-state-toggle-button").click(function()
    {
        id = this.id;
        parent = id.split('-').at(0);
        state = id.split('-').at(1);

        $("#" + parent + "-off").removeClass("active_off");
        $("#" + parent + "-low").removeClass("active_low");
        $("#" + parent + "-high").removeClass("active_high");

        $("#" + id).addClass("active_" + state);
    });

    var new_select = create_select();
    new_select.id = "new_timer_action";
    document.getElementById("new_task_action").appendChild(new_select);

    document.getElementById("new_task_argument").max = max_allowed_temp;
    document.getElementById("target_temp-input").max = max_allowed_temp;

    resize_window();
    get_timers();
    get_state();
    window.onresize = resize_window;
});
