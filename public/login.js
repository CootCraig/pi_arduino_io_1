Zepto(function($){
  $( "#login_submit" ).click(function() {
    var login = '';
    var password = '';
    var url = '';

    login = $('#login_input').val();
    password = $('#password_input').val();
    url = '/login.html?login=';
    url += login;
    url += '&password=';
    url += password;
    window.location.href = url;
  });
});
