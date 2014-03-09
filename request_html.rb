require 'pathname'
require 'haml'
require 'socket'
require 'http-cookie'
require 'cgi'

require_relative './version'

module ReelHttpsAuthWebsock
  class RequestHtml
    include Singleton
    def initialize
      @logger = Log4r::Logger['base::RequestHtml'] || Log4r::Logger['base']
      @public_folder_pathname = App.script_folder_pathname + 'public'
      @logger.info "public_folder_pathname #{@public_folder_pathname}"
    end
    def authenticated_cookie?(request)
      authenticated = false
      begin
        if request.headers['Cookie']
          @logger.debug "authenticated_cookie? cookie #{request.headers['Cookie']}"
          rx = %r{login=(?<login>[^:]+):(?<password>[^;]+)}
          match = rx.match(request.headers['Cookie'])
          if match
            login = match[:login]
            password = match[:password]
            @logger.debug "authenticated_cookie? login #{login} password #{password}"
            if login && password
              authenticated = valididate_login_password(login,password)
              @logger.debug "authenticated_cookie? authenticated #{authenticated} login #{login} password #{password}"
            end
          end
        end
      rescue
        authenticated = false
      end
      authenticated
    end
    def request_page(request)
      if request.path == '/login.html'
        authenticated = false
        query_string = request.query_string || ''
        query_hash = CGI::parse(query_string)
        login = query_hash['login'][0] || ''
        password = query_hash['password'][0] || ''
        authenticated = valididate_login_password(login,password)
        if authenticated
          request.respond :'temporary_redirect', {:Location => '/index.html', :'Set-Cookie' => "login=#{login}:#{password}"}, "Authenticated"
        else
          render_haml(request,{:login => login, :password => password})
        end
      elsif authenticated_cookie?(request)
        if request.path == '/'
          page = 'index.html'
        else
          page = request.path.sub(%r{^/},'')
        end
        render_haml(request,{})
      else
        request.respond :'temporary_redirect', {:Location => '/login.html'}, "Login required"
      end
    end
    def render_haml(request,extra_vars={})
      haml_vars = {:version => ReelHttpsAuthWebsock::VERSION, :hostname => Socket.gethostname}
      haml_vars.merge!(extra_vars)

      if request.path == '/'
        page = 'index.html'
      else
        page = request.path.sub(%r{^/},'')
      end
      page = page.sub(%r{html$},'haml')
      page_pathname = @public_folder_pathname + page
      page_full_path = page_pathname.to_s
      @logger.debug "request_page: render haml from #{page_full_path}"
      if File.exists?(page_full_path)
        begin
          page_engine = Haml::Engine.new(File.read(page_full_path))
          page_html = page_engine.render(Object.new,haml_vars)
          request.respond :ok, page_html
        rescue => ex
          @logger.error "request_page: #{ex.to_s}\n#{ex.backtrace}"
          request.respond :not_found, "Render error"
        end
      else
        request.respond :not_found, "Page not found."
      end
    end
    def valididate_login_password(login,password)
      @logger.debug "valididate_login_password: login #{login} password #{password}"
      valid = false
      begin
        login_sym = login.to_s.downcase.to_sym
        if App.config[:logins] && App.config[:logins].has_key?(login_sym)
          if App.config[:logins][login_sym] == password
            valid = true
          end
        end
      rescue
        valid = false
      end
      @logger.debug "valididate_login_password: valid #{valid} login #{login} password #{password}"
      valid
    end
  end
end
# 307 Temporary Redirect (since HTTP/1.1)
# 307 => 'Temporary Redirect',
# header('HTTP/1.1 301 Moved Permanently');
# header('Location: http://www.example.com/');
# request_page: headers login=zipperhead:yabutta 
# r = %r{login=[^:]+:[^;]+}
# r = %r{login=(?<user>[^:]+):(?<password>[^;]+)}

