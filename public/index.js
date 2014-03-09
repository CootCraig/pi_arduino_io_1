Zepto(function($){
  var server_time_socket = null;
  var websocket_url = null;

  websocket_url = "wss://" + location.host + "/server_time"
  server_time_socket = new WebSocket(websocket_url);

  server_time_socket.onmessage = function (event) {
    $('#current_time').html(event.data);
  };
});

