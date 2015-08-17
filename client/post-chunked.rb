require 'net/http'

Net::HTTP.new('localhost', 10080).start do |http|
  req = Net::HTTP::Post.new ('/test')
  req['Transfer-Encoding'] = 'chunked'
  File.open ('client/post-chunked.rb') do |f|
    req.body_stream = f
    print http.request(req).body
    puts
  end
end
