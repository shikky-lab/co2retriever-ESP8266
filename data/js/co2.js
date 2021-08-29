$(document).ready(function () {
  $.ajax({
    type: 'GET',
    url: 'co2',
    dataType: 'json',
    success: function (data) {
      for (var key in data) {
        $('#' + key).text(data[key]);
      }
    },
    error: function (xhRequest, textStatus, errorThrown) {
      alert("errors in load co2 value. " + textStatus + " : " + errorThrown);
    }
  });
});
