
require 'rubygems'
require 'bundler/setup'

require 'celluloid/autostart'
require 'celluloid/io'

require 'spoon'

class SocatSpawnAndWait
  include 'Celluloid'
  include 'Celluloid::Notifications'

  ACTOR_SYMBOL = :socat_spawn_and_wait
  SPAWN_PATH '/usr/bin/env'
  DEFAULT_SPAWN_ARGV %w(/usr/bin/socat tcp-listen:54321,reuseaddr,fork file:/dev/Diavolino,raw,echo=0,b9600,waitlock=/var/run/Diavolino.lock')

  def initialize(aspawn_argv=DEFAULT_SPAWN_ARGV)
    @spawn_argv = aspawn_argv
  end
  def run
    file_actions = Spoon::FileActions.new
    spawn_attr = Spoon::SpawnAttributes.new
    if true
      spawn_pid = Spoon.posix_spawn(SPAWN_PATH, file_actions, spawn_attr, @spawn_argv)
    else
      begin
        spawn_pid = Spoon.posix_spawn(SPAWN_PATH, file_actions, spawn_attr, @spawn_argv)
      rescue SystemCallError => ex
      rescue => ex
      end
    end
    wait_pid,wait_status = Process.waitpid(spawn_pid)
    [wait_pid,wait_status]
  end
end

class SocatManager
  include 'Celluloid'
  include 'Celluloid::Notifications'
end

class SocketArduinoIo
  # Rejected \a BEL 7 \007
  # Rejected \t TAB 9 \011
  # Rejected \e ESC 27 \033
  #
  # Consider \v Vertical Tab 11 \013
  # Consider \f Form Feed 12 \014

  MESSAGE_START_CHAR = ""
  MESSAGE_TERMINATOR_CHAR  = "\f"
end

class App
end

puts "starting"
span_and_wait = SocatspawnAndWait.new
puts "about to run"
span_and_wait.run

