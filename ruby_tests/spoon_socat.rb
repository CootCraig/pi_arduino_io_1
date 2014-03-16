
require 'rubygems'
require 'bundler/setup'

require 'celluloid/autostart'
require 'celluloid/io'

require 'spoon'

# Run socat in a separate process using Spoon.posix_spawn
# Then block on Process.waitpid2
#
# What are the possible error conditions to test for
# no device file
# bad permissions on device file
# no socat exe
# socat already running
# socat exits
# socat exits socat tcp port busy. 2014/03/15 17:43:54 socat[11203] E bind(3, {AF=2 0.0.0.0:54321}, 16): Address already in use
# SIGHUP        1       Term    Hangup detected on controlling terminal
# socat killed process exits - SocatSpawnAndWait waitpid2 returned exited? true exitstatus 0 signaled? false stopsig 0 termsig 0 success? true
class SocatSpawnAndWait
  include Celluloid
  include Celluloid::Notifications
  finalizer :my_finalizer

  ACTOR_SYMBOL = :socat_spawn_and_wait
  SOCAT_STARTED_TOPIC = 'socat_started_topic'
  SOCAT_ENDED_TOPIC = 'socat_ended_topic'
  SPAWN_PATH = '/usr/bin/env'
  DEFAULT_SPAWN_ARGV = %w(env /usr/bin/socat tcp-listen:54321,reuseaddr,fork file:/dev/Diavolino,raw,echo=0,b9600,waitlock=/opt/run/Diavolino.lock)


  def initialize(aspawn_argv=DEFAULT_SPAWN_ARGV)
    @spawn_argv = aspawn_argv
    @spawn_pid = nil
  end

  def my_finalizer
    Process.kill("HUP", @spawn_pid) if @spawn_pid rescue nil
  end

  def run(started_condition=nil)
    file_actions = Spoon::FileActions.new
    spawn_attr = Spoon::SpawnAttributes.new
    begin
      @spawn_pid = Spoon.posix_spawn(SPAWN_PATH, file_actions, spawn_attr, @spawn_argv)
    rescue SystemCallError => ex
      @spawn_pid = nil # log here? note and report the error?
    rescue => ex
      @spawn_pid = nil
    end
    started_condition.signal(@spawn_pid) if started_condition
    publish(SocatSpawnAndWait::SOCAT_STARTED_TOPIC,@spawn_pid)
    wait_pid,wait_status = Process.waitpid2(@spawn_pid)
    if @spawn_pid.nil? || (wait_pid == @spawn_pid)
      wait_info = [@spawn_pid,wait_status]
    else
      wait_info = [@spawn_pid,nil]
    end
    publish(SocatSpawnAndWait::SOCAT_ENDED_TOPIC,wait_info)
    @spawn_pid = nil
    #puts "SocatSpawnAndWait waitpid2 returned exited? #{wait_status.exited?} exitstatus #{wait_status.exitstatus} signaled? #{wait_status.signaled?} stopsig #{wait_status.stopsig} termsig #{wait_status.termsig} success? #{wait_status.success?}"

    wait_info
  end
end
# manage runing of the socat sub process
class SocatManager
  include Celluloid
  include Celluloid::Notifications

  ACTOR_SYMBOL = :socat_manager

  def initialize
    @socat_pid = nil
    @wait_info = nil
    subscribe(SocatSpawnAndWait::SOCAT_ENDED_TOPIC,:socat_ended)
    SocatSpawnAndWait.supervise_as(SocatSpawnAndWait::ACTOR_SYMBOL)
  end

  def run(caller_started_condition=nil)
    unless running?
      puts "SocatManager.run calling SocatSpawnAndWait.run"
      started_condition = Celluloid::Condition.new
      Celluloid::Actor[SocatSpawnAndWait::ACTOR_SYMBOL].async.run(started_condition)
      @socat_pid = started_condition.wait
    end
    caller_started_condition.signal(@socat_pid) if caller_started_condition
    @socat_pid
  end

  def running?
    if @socat_pid
      pid_found = File.directory?("/proc/#{@socat_pid.to_s}")
      @socat_pid = nil unless pid_found
      pid_found
    else
      false
    end
  end

  def socat_ended(topic,wait_info)
    wait_pid = wait_info[0]
    wait_status = wait_info[1]
    @socat_pid = nil
    @wait_info = wait_info
    if wait_pid.nil? || wait_status.nil?
      puts "SocatManager.socat_ended Unexpected wait status. give up."
      exit 99
    elsif wait_status.exited?
      # socat exited restart it
      puts "SocatManager.socat_ended exited run it again"
      async.run(nil)
    elsif wait_status.signaled?
      # signal 1 seen when socat assigned busy port
      puts "SocatManager.socat_ended with uncaught signal. give up."
      exit 99
    else
      puts "SocatManager.socat_ended Unexpected wait status. give up."
      exit 99
    end
    
  end
end

class ArduinoIo
  include Celluloid::IO

  # Rejected \a BEL 7 \007
  # Rejected \t TAB 9 \011
  # Rejected \e ESC 27 \033
  #
  # Consider \v Vertical Tab 11 \013
  # Consider \f Form Feed 12 \014

  MESSAGE_START_CHAR = "\f"
  MESSAGE_TERMINATOR_CHAR  = "\v"

  ACTOR_SYMBOL = :arduino_io

  def initialize(host, port)
    puts "*** Connecting to Arduino on socket #{host}:#{port}"

    @socket = TCPSocket.new(host, port) # a magic Celluloid::IO socket
    @arduino_buffer = ''

    async.run
  end

  def handle_read(msg_partial)
    # puts "ArduinoIo.handle_read msg_partial [#{msg_partial}]"
    @arduino_buffer += msg_partial
    puts "ArduinoIo.handle_read arduino_buffer [#{@arduino_buffer.dump}]"
    terminator_pos = @arduino_buffer.index(ArduinoIo::MESSAGE_TERMINATOR_CHAR)
    if terminator_pos
      msg_start = @arduino_buffer.slice(0..(terminator_pos-1))
      @arduino_buffer = @arduino_buffer.slice((terminator_pos+1)..(-1))
      start_pos = msg_start.index(ArduinoIo::MESSAGE_START_CHAR)
      if start_pos
        msg = msg_start.slice((start_pos+1)..(-1))
        puts "ArduinoIo.handle_read message [#{msg.dump}] start and stop seen"
        if start_pos > 0
          fragment = msg_start.slice(0..(start_pos-1))
          if fragment.length > 0
            puts "ArduinoIo.handle_read fragment [#{fragment.dump}] fragment before message"
          end
        end
      else
        fragment = msg_start
        puts "ArduinoIo.handle_read fragment [#{fragment.dump}] end seen, but no start"
      end
    end
  end

  def run
    loop do
      begin
        msg = @socket.readpartial(1024)
        async.handle_read(msg)
      rescue EOFError => ex
        # EOFError: End of file reached
        puts "readpartial EOFError exception"
        exit 99
      rescue => ex
        puts "readpartial exception class #{ex.class}"
        exit 99
      end
    end
  end

  def write_to_arduino(msg)
    puts "write <start>#{msg}<end>"
    @socket.write("#{ArduinoIo::MESSAGE_START_CHAR}#{msg}#{ArduinoIo::MESSAGE_TERMINATOR_CHAR}")
    @socket.write(msg)
  end

  def shutdown
    @socket.close
  rescue
  end
end

class AppActor
  include Celluloid
  include Celluloid::Notifications

  def initialize
    @started = nil
    @pid = nil
  end

  def socat_ended(topic,wait_info)
    puts "AppActor.socat_ended old pid #{@pid} ended pid #{wait_info[0]}"
    @pid = nil
  end
  def socat_started(topic,pid)
    if @pid && (@pid == pid)
      puts "AppActor.socat_started same pid #{pid}"
    elsif @pid
      puts "AppActor.socat_started old pid #{@pid} new pid #{pid}"
    else
      puts "AppActor.socat_started old pid nil new pid #{pid}"
    end
    @pid = pid
  end
  def start_socat_test
    @pid = nil
    subscribe(SocatSpawnAndWait::SOCAT_STARTED_TOPIC,:socat_started)
    subscribe(SocatSpawnAndWait::SOCAT_ENDED_TOPIC,:socat_ended)

    SocatManager.supervise_as(SocatManager::ACTOR_SYMBOL)
    started_condition = Celluloid::Condition.new
    puts "AppActor.start_socat_test Request that SocatManager run socat"
    Celluloid::Actor[SocatManager::ACTOR_SYMBOL].async.run(started_condition)
    puts "AppActor.start_socat_test wait for start"
    started_condition.wait
    puts "AppActor.start_socat_test wait for start returned"
    every (5) do
      socat_running = Celluloid::Actor[SocatManager::ACTOR_SYMBOL].running?
      puts "AppActor socat running? #{socat_running}"
    end
  end

  def start_arduino_io_test
    puts "AppActor.start_arduino_io_test start SocatManager"
    SocatManager.supervise_as(SocatManager::ACTOR_SYMBOL)
    started_condition = Celluloid::Condition.new
    puts "AppActor.start_arduino_io_test Request that SocatManager run socat"
    Celluloid::Actor[SocatManager::ACTOR_SYMBOL].async.run(started_condition)
    puts "AppActor.start_arduino_io_test wait for start"
    started_condition.wait
    puts "AppActor.start_arduino_io_test wait for start returned"
    puts "AppActor.start_arduino_io_test Start ArduinoIo"
    ArduinoIo.supervise_as(ArduinoIo::ACTOR_SYMBOL,'localhost',54321)
    PrefixTimer.new
  end
end

class PrefixTimer
  include Celluloid

  ACTOR_SYMBOL = :prefix_timer

  def initialize
    @counter = 1
    every (30) do
      @counter += 1
      Celluloid::Actor[ArduinoIo::ACTOR_SYMBOL].async.write_to_arduino("p#{@counter}")
    end
  end
end

app = AppActor.new
if false
  app.start_socat_test
else
  app.start_arduino_io_test
end
sleep

