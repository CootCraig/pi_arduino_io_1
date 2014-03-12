require 'log4r'

module PiArduinoIo1
  class RequestJs
    include Singleton
    def initialize
      @logger = Log4r::Logger['base::RequestJs'] || Log4r::Logger['base']
      @public_folder_pathname = App.script_folder_pathname + 'public'
    end
    def request_js(request)
      js_filename = request.path.sub(%r{^/},'')
      js_pathname = @public_folder_pathname + js_filename
      js_full_path = js_pathname.to_s
      if File.exists?(js_full_path)
        @logger.debug "request_js: js path found #{js_full_path}"
        begin
          js_content = File.read(js_full_path)
        rescue => ex
          @logger.error "request_js: read error #{ex.to_s}\n#{ex.backtrace}"
          js_content = ''
        end
      else
        @logger.debug "request_js: js path NOT found #{js_full_path}"
        js_content = ''
      end
      request.respond :ok, js_content
    end
  end
end
