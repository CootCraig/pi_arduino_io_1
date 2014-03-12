require 'celluloid/io'
require 'cgi'
require 'json'
require 'log4r'

module PiArduinoIo1
  class ArduinoIo
    include Celluloid::IO
    finalizer :shutdown

    MESSAGE_TERMINATOR_CHAR  = "\f"

    def initialize(ahost,aport)
      @logger = Log4r::Logger['base::ArduinoIo'] || Log4r::Logger['base']
      @arduino_io_host = ahost
      @arduino_io_port = aport
      @logger.info "Connecting to RPi IO socket on #{@arduino_io_host}:#{@arduino_io_port}"

      @socket = TCPSocket.new(@arduino_io_host, @arduino_io_port) # a magic Celluloid::IO socket
      @arduino_buffer = ''

      async.run
    end

    def handle_read(msg)
      line = ''
      @logger.debug "handle_read msg [#{msg}]"
      @arduino_buffer += msg
      @logger.debug "arduino_buffer [#{@arduino_buffer}]"
      terminator_pos = @arduino_buffer.index(ArduinoIo::MESSAGE_TERMINATOR_CHAR)
      if terminator_pos
        line = @arduino_buffer.slice(0..(terminator_pos-1))
        @arduino_buffer = @arduino_buffer.slice((terminator_pos+1)..-1)
        @logger.debug "message line [#{line}]"
        Celluloid::Actor[:message_server].async.publish(line)
      end
    end

    def run
      loop do
        msg = @socket.readpartial(1024)
        async.handle_read(msg)
      end
    end

    def shutdown
      @socket.close
    rescue
    end

    def write_to_arduino(msg)
      @logger.debug "write_to_arduino #{msg}"
      @soc
      @socket.write(msg + ArduinoIo::MESSAGE_TERMINATOR_CHAR)
    end

    def self.send_request(request)
      query_string = request.query_string || ''
      params = CGI::parse(query_string)
      if params.has_key?('message') && (params['message'].length > 0) && (params['message'][0].length > 0)
        message = params['message'][0]
        Celluloid::Actor[:arduino_io].async.write_to_arduino(message)
        response_body = { :status => 'success', :log => 'sent' }
        headers = {}
      else
        response_body = { :status => 'fail', :log => 'Message required' }
        headers = {}
      end
      connection.respond :ok, headers, response_body.to_json
    rescue => ex
      response_body = { :status => 'fail', :log => 'Server error' }
      headers = {}
      connection.respond :internal_server_error, headers, response_body.to_json
    end
  end
end

