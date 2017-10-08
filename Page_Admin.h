

//
//  HTML PAGE
//

const char PAGE_AdminMainPage[] PROGMEM = R"=====(
<meta name="viewport" content="width=device-width, initial-scale=1" />
<strong>Administration</strong>
<hr>
<a href="general.html" style="width:250px" class="btn btn--m btn--blue" >General Configuration</a><br>
<a href="config.html" style="width:250px" class="btn btn--m btn--blue" >Network Configuration</a><br>
<a href="info.html"   style="width:250px"  class="btn btn--m btn--blue" >Network Information</a><br>
<a href="ntp.html"   style="width:250px"  class="btn btn--m btn--blue" >NTP Settings</a><br>
<a href="cccam.html"   style="width:250px"  class="btn btn--m btn--blue" >CCCam Settings</a><br>
<a href="firmware"   style="width:250px"  class="btn btn--m btn--blue" >Update Firmware</a><br>
<a href="/"   style="width:250px"  class="btn btn--m btn--blue" >copyright©2016</a><br>
<span id="uptime" style="font-size:14px;color:#E56600;" </span> 


<script>

function Getuptime()
{
  setValues("/admin/uptime");
}

window.onload = function ()
{
	load("style.css","css", function() 
	{
		load("microajax.js","js", function() 
		{
				Getuptime();
		});
	});
}
function load(e,t,n){if("js"==t){var a=document.createElement("script");a.src=e,a.type="text/javascript",a.async=!1,a.onload=function(){n()},document.getElementsByTagName("head")[0].appendChild(a)}else if("css"==t){var a=document.createElement("link");a.href=e,a.rel="stylesheet",a.type="text/css",a.async=!1,a.onload=function(){n()},document.getElementsByTagName("head")[0].appendChild(a)}}

</script>

)=====";


void send_information_uptime_html ()
{

	int sec = millis() / 1000;
  int minutes = sec / 60;
  int hr = minutes / 60;
  
  char temp[64];
  
  snprintf ( temp, 64,"<p>&nbsp; &nbsp; &nbsp; &nbsp; Uptime: %02d:%02d:%02d</p>",hr, minutes % 60, sec % 60);
  
  String values ="";

  values += "uptime|" + (String)temp +  "|div\n";
  server.send ( 200, "text/plain", values);
  // Serial.println(__FUNCTION__); 
  AdminTimeOutCounter=0;
}


