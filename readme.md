This is a minor modification of the text elite source from Ian Bells
website.

As there was no licence included with the source, it is to be presumed
that it is in the public domain, which includes the minor modification
made by myself

I found that it wouldn't compile cleanly on "modern" compilers
clang being the worst(?) objector, which warned about some lines that
seemed to have no purpose or side effect.

Commenting out these lines appears to have no effect on the behaviour
compared with the behaviour of the original source.

With the original source, the goat soup routine wouldn't recognise 
tokens because of sign expansion making the value invalid, masking the
value with 0xFF seems to be sufficient to fix this...

from the original website

	"You can send scripts to txtelite.exe with a command such as 
	txtelite < spears.txt but take care to ensure that a final "quit" is 
	present in the file. The drug- and fur-running Commander Spears 
	submitted by Christian Pinder, achieves 5016 CR in just 23 commands. 
	Another wicked peddlar of alien narctoics, Commander Sinclair , 
	submitted by Duane McDonnell, achieves 1001201 CR with just 632 
	hyperspace jumps."

even with the original sources (or precompiled EXE with dosbox) I 
couldn't get spears.txt to produce 5016 CR.  However sinclair.txt does
manage to produce the expected 1001201 (+ .2!) CR

I've spent time making sure this works properly on modern compilers in
memory of the many hours I spent with an amber screen monitor and a
BBC B, in those day there was no game with such a massive open sandbox
so far ahead of its time, which inspired so many interesting games...

Codifies
http://bedroomcoders.co.uk/captcha/
