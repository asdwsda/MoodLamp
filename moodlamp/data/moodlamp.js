var leds = [
    {r: 0, g:0, b:0},
    {r: 0, g:0, b:0}
];

function disabled(id, state) {
    document.getElementById(id).disabled = state;
}

function rainbow(action) {
    sendRequest("rainbow", {"action": action}, null);
}

function sendRequest(command, params, callback) {
    var xh = new XMLHttpRequest();
    var get_params = "";
    for (key in params) {
        get_params += key + "=" + params[key] + "&"
    }
    xh.onreadystatechange = function() {
        if (xh.readyState == 4) {
            console.log(xh.responseText);
            if (callback != null)
                callback(xh)
        }
    }
    xh.open("GET", "/" + command + "?" + get_params);
    xh.send(null);
}

function updateSliderVal(slider) {
    elem = document.getElementById(slider.dataset.channel + "_" + slider.dataset.led_id + "_val");
    elem.innerHTML = slider.value;
}

function changeColor(slider) {
    var id = slider.dataset.led_id
    var channel = slider.dataset.channel
    var before = leds[id][channel];
    leds[id][channel] = slider.value;

    var params = {};
    params['r'] = leds[id]['r'];
    params['g'] = leds[id]['g'];
    params['b'] = leds[id]['b'];
    params['id'] = id;

    slider.disabled = true;
    sendRequest("color", params, function(xh) {
        if (xh.status != "200") {
            slider.value = before;
            leds[id][channel] = before;
            updateSliderVal(slider);
        }
        slider.disabled = false;
    });
}
