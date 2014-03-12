require 'celluloid'

module PiArduinoIo1
  class MessageServer
    include Celluloid

    def initialize
      @logger = Log4r::Logger['base::MessageServer'] || Log4r::Logger['base']
      @websocket_clients = {}
      @logger.info "Starting"
    end
    # index.js uses path /messages, but all websocket requests go here for now.
    def add_websocket(websocket)
      @websocket_clients[websocket.object_id] = websocket unless @websocket_clients[websocket.object_id]
    end
    def publish(arduino_message)
      @websocket_clients.each_pair do |key,websocket|
        begin
          websocket << arduino_message
        rescue => ex
          begin
            @websocket_clients.delete(key)
          rescue => ex2
          end
        end
      end
    end
  end
end
