print("Servo control")

-- put module in AP mode
wifi.setmode(wifi.SOFTAP)
print("ESP8266 mode is: " .. wifi.getmode())
cfg={}
 
-- Set the SSID of the module in AP mode and access password
cfg.ssid="ESP_STATION"
cfg.pwd="mypassword"
print("ESP8266 SSID is: " .. cfg.ssid .. " and PASSWORD is: " .. cfg.pwd)

servopinLR = 4
pwm.setup(servopinLR,50,75) 
pwm.start(servopinLR)
servopinDU = 3
pwm.setup(servopinDU,50,75) 
pwm.start(servopinDU)

wifi.ap.config(cfg)
ap_mac = wifi.ap.getmac()

-- create a server on port 80 and wait for a connection, when a connection is coming in function c will be executed
sv=net.createServer(net.TCP,30)
sv:listen(80,function(c)
 c:on("receive", function(c, pl)
 --print(pl) -- print the payload pl received from the connection
 --print(string.len(pl))
 --print(string.match(pl,"GET"))
 servolr_start,servolr_end=string.find(pl,"servolr=")

 if servolr_start and servolr_end then
   amper_start, amper_end =string.find(pl,"&", servolr_end+1)
   if amper_start and amper_end then
     http_start, http_end =string.find(pl,"HTTP/1.1", servolr_end+1)
     if http_start and http_end then
       servolr=string.sub(pl,servolr_end+1, amper_start-1)
       servodu=string.sub(pl,amper_end+9, http_start-2)
       print("Servo LeftRight: " .. servolr .. " DownUp: " .. servodu)
	   a0 = tonumber(servolr)
	   a1 = tonumber(servodu)
	   print("a0:" .. a0)
	   print("a1:" .. a1)
	   pwm.setduty(servopinLR,a0)
	   pwm.setduty(servopinDU,a1)
    end
   end
  end

  c:send("<!DOCTYPE html>")
  c:send("<html>")
  c:send("<body>")
  c:send("<form action='' method='get' align='center'>")
  c:send("Servo Control:<br />")
  c:send("<input id='SliderLR' type='range' name='servolr' orient='horizontal' min='0' max='300' ")
  c:send("  style='width: 600px; height: 100px; -webkit-appearance: slider-horizontal; writing-mode: bt-lr;' onChange='SliderLRFunc(this.value)'></input><br />")
  c:send("<input id='SliderDU' type='range' name='servodu' orient='vertical' min='0' max='300' ")
  c:send("  style='width: 600px; height: 600px; -webkit-appearance: slider-vertical; writing-mode: bt-lr;' onChange='SliderDUFunc(this.value)'></input><br />")
  c:send("<canvas id='myCanvas' width='600' height='600' style='background: #0488FB; border:2px solid #d3d3d3;'></canvas>")
  c:send("</form>")
  c:send("<script>")
  c:send("var canvas = document.getElementById('myCanvas');")
  c:send("var ctx = canvas.getContext('2d');")
  c:send("ctx.font = '30px Comic Sans MS';")
  c:send("ctx.fillStyle = 'blue';")
  c:send("ctx.textAlign = 'center';")
  c:send("ctx.fillText('Touch!', canvas.width/2, canvas.height/2); ")
  c:send("canvas.addEventListener('mousedown', doMouseDown, false);")
  c:send("document.getElementById('SliderLR').value = getQueryParams(document.location.search).servolr;")
  c:send("document.getElementById('SliderDU').value = getQueryParams(document.location.search).servodu;")
  c:send("function SliderLRFunc(val) {window.location = '?servolr='+val+'&servodu='+document.getElementById('SliderDU').value;}")
  c:send("function SliderDUFunc(val) {window.location = '?servolr='+document.getElementById('SliderLR').value+'&servodu='+val;}")
  c:send("function getQueryParams(qs) {")
  c:send("    qs = qs.split('+').join(' ');")
  c:send("    var params = {},")
  c:send("        tokens,")
  c:send("        re = /[?&]?([^=]+)=([^&]*)/g;")
  c:send("    while (tokens = re.exec(qs)) {")
  c:send("        params[decodeURIComponent(tokens[1])] = decodeURIComponent(tokens[2]);}")
  c:send("    return params;}")
  c:send("function doMouseDown(event) {")
  c:send("  canvas_x = event.pageX - canvas.offsetLeft;")
  c:send("  canvas_y = event.pageY - canvas.offsetTop;")
  c:send("  window.location = '?servolr='+(canvas_x/2)+'&servodu='+((600-canvas_y)/2);}")
  c:send("</script>")
  c:send("</body>")
  c:send("</html>")
  end)
end)
