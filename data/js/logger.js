
$(document).ready(function () {
    $.PeriodicalUpdater('log', {
        method: 'get',
        minTimeout: 1000,
        type: 'text',
    },
        function (data) {
            $('#log-dev').html(data);
        });
});
