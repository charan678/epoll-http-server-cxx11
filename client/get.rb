#!/usr/bin/env ruby

require 'net/http'

Net::HTTP.new('localhost', 10080).start do |http|
  http.request_get('/') do |res|
    body = ''
    res.read_body {|s| body << s }
    puts '---'
    puts 'content-type: ' + res['content-type'].inspect
    puts
    print body
  end
  http.request_get('/about.html', {'Connection' => 'close'}) do |res|
    body = ''
    res.read_body {|s| body << s }
    puts '---'
    puts 'content-type: ' + res['content-type'].inspect
    puts
    print body
  end
end

