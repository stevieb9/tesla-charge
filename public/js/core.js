"use strict";

var interval = 1000;  // 1000 = 1 second, 3000 = 3 seconds
setTimeout(doAjaxState, interval);
setTimeout(doAjaxManualMode, interval);

$(document).ready(function(){

    $("#garage-door").click(function() {
        var post_data = $.ajax({
            url: "/garage_door_action_set",
            type: 'POST',
            contentType: 'application/json; charset=utf-8',
            success: function() {
            },
        });
    });

    $("#manual-mode").click(function() {
        var post_data = $.ajax({
            url: "/garage_door_manual_set",
            type: 'GET',
            success: function() {
            },
        });
    });

});

function doAjaxState() {
    $.ajax({
            type: 'GET',
            url: '/garage_door_state',
            success: function (doorState) {
                
                console.log(parseInt(doorState));
                var $garage_door = $('#garage-door');

                if ($('#manual-mode').prop('value') === "On") {
                    $garage_door.prop('disabled', false);
                    if (parseInt(doorState) === 1) {
                        $garage_door.prop('value', 'Close');
                    }
                    else {
                        $garage_door.prop('value', 'Open');
                    }
                }
                else {
                    $garage_door.prop('value', 'Unavailable');
                    $garage_door.prop('disabled', true);
                }

            },
            complete: function (data) {
                    setTimeout(doAjaxState, interval);
            }
    });
}

function doAjaxManualMode() {
    $.ajax({
            type: 'GET',
            url: '/garage_door_manual',
            success: function (manualState) {
                var $garage_door = $('#garage-door');

                if (parseInt(manualState) === 1) {
                    $('#manual-mode').prop('value', 'On');
                    $garage_door.prop('disabled', false);
                }
                else {
                    $('#manual-mode').prop('value', 'Off');
                    $garage_door.prop('disabled', true);
                }
            },
            complete: function (data) {
                    setTimeout(doAjaxManualMode, interval);
            }
    });
}
