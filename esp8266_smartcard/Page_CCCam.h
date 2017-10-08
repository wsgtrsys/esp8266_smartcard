//
//  HTML PAGE
//
const char PAGE_CCCam_Settings[] PROGMEM =  R"=====(
<meta name="viewport" content="width=device-width, initial-scale=1" />
<meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
<a href="/"  class="btn btn--s"><</a>&nbsp;&nbsp;<strong>CCCam Settings</strong>
<hr>
<form action="" method="post">
<table border="0"  cellspacing="0" cellpadding="3" style="width:310px" >
<tr><td align="right">Server :</td><td><input type="text" id="CCCam_server" name="CCCam_server" value=""></td><tr>
<tr><td align="right">Port   :</td><td><input type="text" id="CCCam_port" name="CCCam_port" value=""></td><tr>
<tr><td align="right">UserName:</td><td><input type="text" id="CCCam_user" name="CCCam_user" value=""></td><tr>
<tr><td align="right">PassWord:</td><td><input type="text" id="CCCam_pass" name="CCCam_pass" value=""></td><tr>
<tr><td align="right">BaudRate:</td><td><input type="text" id="POrt_Bits" name="POrt_Bits" value=""></td><tr>
<tr><td align="right">Card SN:</td><td><input type="text" id="cardid" name="cardid" value=""></td><tr>

<tr><td colspan="2" align="center"><input type="submit" style="width:150px" class="btn btn--m btn--blue" value="Save"></td></tr>
</table>
</form>
<script> 

window.onload = function ()
{
	load("style.css","css", function() 
	{
		load("microajax.js","js", function() 
		{
				setValues("/admin/cccamvalues");
		});
	});
}
function load(e,t,n){if("js"==t){var a=document.createElement("script");a.src=e,a.type="text/javascript",a.async=!1,a.onload=function(){n()},document.getElementsByTagName("head")[0].appendChild(a)}else if("css"==t){var a=document.createElement("link");a.href=e,a.rel="stylesheet",a.type="text/css",a.async=!1,a.onload=function(){n()},document.getElementsByTagName("head")[0].appendChild(a)}}



</script>
)=====";



void send_CCCam_general_html()
{
	
	if (server.args() > 0 )  // Save Settings
	{	
		String temp = "";
		for ( uint8_t i = 0; i < server.args(); i++ ) {
			if (server.argName(i) == "CCCam_server") config.CCCam_server = urldecode(server.arg(i)); 
			if (server.argName(i) == "CCCam_port") config.CCCam_port = server.arg(i).toInt()>65535?3000:server.arg(i).toInt(); 
			if (server.argName(i) == "CCCam_user") config.CCCam_user = urldecode(server.arg(i)); 
			if (server.argName(i) == "CCCam_pass") config.CCCam_pass = urldecode(server.arg(i)); 
			if (server.argName(i) == "POrt_Bits") config.POrt_Bits = server.arg(i).toInt(); 
			if (server.argName(i) == "cardid") config.Card_id = urldecode(server.arg(i)); 
		}
		WriteConfig();
		firstStart = true;
	}
	server.send_P ( 200, "text/html", PAGE_CCCam_Settings ); 
	// Serial.println(__FUNCTION__); 
	
	
}

void send_CCCam_configuration_values_html()
{
	String values ="";
	values += "CCCam_server|" +  (String)  config.CCCam_server +  "|input\n";
	values += "CCCam_port|" +  (String)  config.CCCam_port +  "|input\n";
	values += "CCCam_user|" +  (String)  config.CCCam_user +  "|input\n";
	values += "CCCam_pass|" +  (String)  config.CCCam_pass +  "|input\n";
	values += "POrt_Bits|" +  (String)  config.POrt_Bits +  "|input\n";
	values += "cardid|" +  (String)  config.Card_id +  "|input\n";
 
	server.send ( 200, "text/plain", values);
	// Serial.println(__FUNCTION__); 
    AdminTimeOutCounter=0;
}
