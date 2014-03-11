require 'rubygems'
require 'bundler/setup'

require 'celluloid/io'
require 'celluloid/autostart'

class PiSock
  include Celluloid::IO
  finalizer :shutdown

    def initialize(host, port)
      puts "*** Connecting to RPi on #{host}:#{port}"

      @socket = TCPSocket.new(host, port) # a magic Celluloid::IO socket

      async.run
    end

    def handle_read(msg)
      puts ("[#{msg}]")
    end

    def run
      loop do
        msg = @socket.readpartial(1024)
        async.handle_read(msg)
      end
    end

    def write_to_pi(msg)
      puts "write #{msg}"
      @socket.write(msg)
    end

    def shutdown
      @socket.close
    rescue
    end
end

class PrefixTimer
  include Celluloid

  def initialize
    counter = 1
    every (20) do
      counter += 1
      Celluloid::Actor[:pi_sock].async.write_to_pi("p#{counter}")
    end
  end
end

PiSock.supervise_as(:pi_sock,'localhost',54321)
PrefixTimer.supervise_as(:prefix_timer)
puts "Actors started"
sleep

