const char html_head[] =
"<!DOCTYPE html>"
"<html>"
"<head>"
"<style>"
"body{background:#FFF;color:#000;font-family:verdana,sans-serif;margin:0px}"
".u{border-bottom:1px solid #EEE}"
".u>div{padding:5px 0px 0px 0px;display:inline-block;"
"height:inherit;line-height:20px;font-size:16px;vertical-align:top;width:calc(100% - 50px)}"
".u>div:first-child{width:50px;text-align:center;font-size:12px;line-height:normal}"
".text{word-wrap:break-word;width:500px;height:100px;font-family:verdana,sans-serif;font-size:14px;margin:0px 0px 5px 0px}"
"form{display:inline}"
""
"a{text-decoration:none;}"
"a:hover.a,a:hover.b,label:hover,a:hover.h{text-decoration:underline;color:#D00}"
".a,.f,label,.h{color:#369}"
".b,.c,.d,.e,.i,.j,.u .t,.p,.q{color:#888}"
".c{font-size:10px}"
".d{font-size:12px}"
".e{font-size:9px;}"
".k{font-size:14px;word-wrap:break-word}"
".f,.g{margin:0px 1px}"
".f{padding:1px 5px 1px 5px;}"
".g{padding:0px 4px 1px 4px;color:#F00;background:#FFF;border:1px solid #6AD;border-bottom:none}"
"a:hover.e,a:hover.i{text-decoration:underline}"
".e,.h,.i{font-weight:bold}"
".j{font-style:italic}"
".b1{color:#000}"
".b2{color:#D00}"
".w{word-wrap:break-word}"
""
"label{cursor:pointer}"
".l,.m,.n,.o,.p,.q,.r,.s,.t,.r1,.s1{display:none}"
".r1,.r{color:#D50}"
".s1,.s{color:#55F}"
""
".u .p, .u .q, .u .r, .u .s{font-size:0px;padding:10px;line-height:20px}"
".u .p{background-image:url(\"data:image/svg+xml;base64,PHN2ZyB4bWxucz0iaHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmciIHdpZHRoPSIyMCI+PHBhdGggZmlsbD0iI0FBQSIgZD0iTTAsMTYgMTAsMCAyMCwxNiAxMCwxMnoiLz48L3N2Zz4=\")}"
".u .q{background-image:url(\"data:image/svg+xml;base64,PHN2ZyB4bWxucz0iaHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmciIHdpZHRoPSIyMCI+PHBhdGggZmlsbD0iI0FBQSIgZD0iTTAsNCAxMCwyMCAyMCw0IDEwLDh6Ii8+PC9zdmc+\")}"
".u .r{background-image:url(\"data:image/svg+xml;base64,PHN2ZyB4bWxucz0iaHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmciIHdpZHRoPSIyMCI+PHBhdGggZmlsbD0iI0Y5NiIgZD0iTTAsMTYgMTAsMCAyMCwxNiAxMCwxMnoiLz48L3N2Zz4=\")}"
".u .s{background-image:url(\"data:image/svg+xml;base64,PHN2ZyB4bWxucz0iaHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmciIHdpZHRoPSIyMCI+PHBhdGggZmlsbD0iIzk5RiIgZD0iTTAsNCAxMCwyMCAyMCw0IDEwLDh6Ii8+PC9zdmc+\")}"

".l:checked~.p,.l:checked~.q,.l:checked~.t,.m:checked~.p,.m:checked~.q,.m:checked~.t,.n:checked~.r,.n:checked~.q,.n:checked~.r1,.o:checked~.p,.o:checked~.s,.o:checked~.s1{display:inline}"
""
".y{margin-left:50px}"
".z,.x,.x1,.x2,.y1,.y2{display:none}"
".y1{width:0px}"
".x:checked~.w{display:none}"
".x:checked~.z,.x1:checked~.y2,.x2:checked~.y1{display:inline}"

"p{margin:0}"
"p{display:inline}"
"p{font-family:monospace}"
"q:before,q:after{content:none}"
"q{text-decoration:overline}"
"tt{font-family:inherit}"
"tt{color:#000}"
"tt{background:#000}"
"tt:hover{color:#FFF}"
"</style>"
;

const char html_body[] =
"</head>"
"<body>"
"<iframe name=\"h\" style=\"display:none\"></iframe>"
"<div style=\"background:#cee3f8;padding:3px 0px 0px 5px;border-bottom:1px solid #6AD\">"
;

const char html_submit[] =
"<form method=\"POST\">"
"Title: <input name=\"a\" type=\"text\" maxlength=\"128\"><br>"
"URL: <input name=\"b\" type=\"text\" maxlength=\"256\"><br>"
"<input type=\"submit\" value=\"Submit\">"
"</form>"
"<br>or<br>"
"<form method=\"POST\">"
"Title: <input name=\"a\" type=\"text\" maxlength=\"128\"><br>"
"Text: <textarea class=\"text\" name=\"c\" maxlength=\"2048\"></textarea><br>"
"<input type=\"submit\" value=\"Submit\">"
"</form>"
;

const char html_login_head[] =
"<html>"
"<head><title>/login</title></head>"
"<body>"
"<form method=\"POST\">"
"Login or register<br>"
"Username: <input name=\"a\" type=\"text\" maxlength=\"12\"><br>"
"Password: <input name=\"b\" type=\"password\" maxlength=\"256\"><br>"
"<input type=\"submit\" value=\"Submit\"/>"
"</form>"
"<br><a href=\"/\">home</a>"
;

const char html_end[] =
"</body>"
"</html>"
;

const char html_main_end[] =
"</table>"
"</body>"
"</html>"
;

const char html_post_end[] =
"</div>"
"</body>"
"</html>"
;


