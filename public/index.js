Zepto(function($){
  var server_time_socket = null;
  var websocket_url = null;

  websocket_url = "wss://" + location.host + "/messages"
  server_time_socket = new WebSocket(websocket_url);

  server_time_socket.onmessage = function (event) {
    var message = null;
    var remove_chars_regex = /[\x00-\x1f]/;

    message = event.data.replace(remove_chars_regex,'');
    if ($('li').length >= 15) {
      $('li').last().remove();
    }
    $('#message_list').prepend('<li class="message_li">' + message + '</li>');
  };

  $( "#prefix_submit" ).click(function() {
    var prefix = null;
    prefix = $('#prefix_input').val();
    $.ajax({
      'url': '/send',
      'data': {
        'message': prefix
      },
      'dataType': 'json',
      'complete': function(jqXHR,textStatus) {
        var stat = textStatus;
      },
    });
  });
});

