require 'celluloid'

module ReelHttpsAuthWebsock
  class TimeServer
    include Celluloid

    def initialize
      @logger = Log4r::Logger['base::TimeServer'] || Log4r::Logger['base']
      @websocket_clients = {}
      @logger.info "Starting"
      every(2) { publish }
    end
    def add_websocket(websocket)
      @websocket_clients[websocket.object_id] = websocket unless @websocket_clients[websocket.object_id]
    end
    def publish
      @websocket_clients.each_pair do |key,websocket|
        begin
          websocket << Time.now.inspect
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
