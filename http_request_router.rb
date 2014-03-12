require 'pathname'
require 'haml'

require_relative './arduino_io'
require_relative './request_html'
require_relative './request_js'

module PiArduinoIo1
  class HttpRequestRouter
    include Singleton
    def initialize
      @logger = Log4r::Logger['base::HttpRequestRouter'] || Log4r::Logger['base']
      @public_folder_pathname = App.script_folder_pathname + 'public'
      @logger.info "public_folder_pathname #{@public_folder_pathname}"
    end
    def request_page(request)
      @logger.debug "path #{request.path}"
      case request.path
      when '/send' # /send?message=prefix
        ArduinoIo::send_request(request)
      when %r{js$}
        RequestJs.instance.request_js(request)
      when %r{^/$|html$}
        RequestHtml.instance.request_page(request)
      when %r{css$}
        request.respond :ok, "css path #{request.path}"
      when '/debug'
        request.respond :ok, "/debug uri #{request.uri} headers #{request.headers}"
      else
        request.respond :not_found, "Not Found in request router path #{request.path}"
      end
    end
  end
end
