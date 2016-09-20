honpurple
=========

libpurple plugin for Heroes of Newerth chat server
This is a chat plugin for libpurple-based messengers
pidgin, finch, adium , telepathy, minbif, etc

Install instructions:

Windows:

1) install pidgin from pidgin.im  
2) download win32 zip and unpack it into pidgin's installation directory
In case there is no HoN protocol you may need to download openssl.zip and unpack it int pidgin's directory

<del>Mac OSX:

1) have adium 1.4b18 or newer  
1.1) download honpurple–adium zip from homepage  
2) double click zip  
3) double click plugin  </del>  
I can't make a build of adium version due to not having a mac and there is none uptodate.

Gentoo linux :  
<code>layman -o https://raw.github.com/theli-ua/overlay/master/overlay.xml -f --add honpurple</code>

<b>Downloads</b> 
Latest released packages are now broken because of protocol updates. Git version is working though if you are willing to build it yourself
~~
[Source Code](http://dl.dropbox.com/u/4443078/HoN/honpurple/honpurple-0.5.11.6.tar.bz2)  
[windows binary](http://dl.dropbox.com/u/4443078/HoN/honpurple/honpurple-win-0.5.11.6.zip)  
[windows openssl binary](http://dl.dropbox.com/u/4443078/HoN/honpurple/openssl.zip)  
[x86_64 deb package](http://dl.dropbox.com/u/4443078/HoN/honpurple/honpurple_x86_64-0.5.11.6.deb)
~~
[homepage](https://github.com/theli-ua/honpurple)  
report bugs and find downloads @ homepage

if plugin says 'you have been disconnected .. bla bla bla' right from start that probably means that the protocol version is incorrect... find it in account settings -> advanced


(the whole reason for that setting is not to have to rebuild plugin each time the only thing needed is protocol version)
