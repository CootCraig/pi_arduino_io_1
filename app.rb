require 'rubygems'
require 'bundler/setup'

require 'celluloid/autostart'
require 'log4r'
require 'log4r/yamlconfigurator'
require 'nesty'

require_relative './http_server'
require_relative './time_server'
require_relative './version'

module ReelHttpsAuthWebsock

  # Root exception for ReelHttpsAuthWebsock.
  #
  # Nesty is handy to embed an underlying exception.
  class Error < StandardError
    include Nesty::NestedError
  end

  class App
    @@yaml_config = nil
    @@app_config_file = nil
    @@config = nil
    @@http_server = nil
    @@logger = nil
    @@script_folder_pathname = nil
    @@tls_certificate = nil
    @@tls_key = nil

    # Return the application config hash.
    # :http_host::   Interface used for HTTP
    # :http_port::   Port used for HTTP
    # @return [hash] the application config.
    def self.config
      @@config
    end

    def self.http_server
      @@http_server
    end

    # @!visibility private
    # Read the config file for application options.
    # Log4r logging is initialized from the config file.
    def self.init
      @@config = {}
      @@script_folder_pathname = Pathname.new(File.expand_path(File.dirname(__FILE__)))
      @@app_config_file = (@@script_folder_pathname + 'app.yml').to_s
      puts "app_config_file [#{@@app_config_file}]"
      Log4r::YamlConfigurator.load_yaml_file @@app_config_file

      @@logger = Log4r::Logger['base::App'] || Log4r::Logger['base']
      @@logger.info "\n\n========= Startup version #{ReelHttpsAuthWebsock::VERSION} ========="
      @@logger.info "Log4r and application configured from file #{@@app_config_file}"

      begin
        @@yaml_config = YAML.load_file(@@app_config_file)

        @@config[:http_host] = @@yaml_config['http_host']
        @@config[:http_port] = @@yaml_config['http_port']
        @@logger.info "http_host #{@@config[:http_host]} http_port #{@@config[:http_port]}"

        @@config[:logins] = {}
        @@yaml_config['logins'].each do |login,password|
          @@config[:logins][login.downcase.to_sym] = password
          @@logger.info "login #{login} password #{password}"
        end

        load_tls
      rescue => ex
        @@logger.error "#{ex.to_s}\n#{ex.backtrace}"
      end
    end
    private_class_method :init

    def self.load_tls
      cert_file = (@@script_folder_pathname + 'tls/ca.crt').to_s
      key_file = (@@script_folder_pathname + 'tls/ca.key').to_s
      unless File.exist?(cert_file) && File.exist?(key_file)
        raise Error.new("cert files not found")
      end
      @@tls_certificate = File.read(cert_file)
      @@tls_key = File.read(key_file)
      @@logger.info "tls certificate\n#{@@tls_certificate}"
      @@logger.info "tls key\n#{@@tls_key}"
    end

    def self.logger
      @@logger
    end

    def self.script_folder_pathname
      @@script_folder_pathname
    end

    def self.run
      init
      TimeServer.supervise_as(:time_server)
      @@logger.info "TimeServer started"
      @@http_server = HttpServer.new(@@config[:http_host],@@config[:http_port],@@tls_certificate,@@tls_key)
      @@logger.info "HttpServer started"
      puts "https started"
      sleep
    end

  end
end

ReelHttpsAuthWebsock::App.run

