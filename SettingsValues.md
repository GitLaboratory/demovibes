For general django settings, consult the [Django documentation](http://docs.djangoproject.com/en/dev/ref/settings).

**SONGS\_IN\_QUEUE**

> Defines how many songs one user can have in the queue at the same time.

**SONG\_LOCK\_TIME**

> Defines how long a song will be locked for queuing after being requested (or played randomly).

**UPLOADED\_SONG\_COUNT**

> Defines how many uploaded songs are displayed in the 'Recently Uploaded' category.

**PLAY\_JINGLES**

> If this is set to True, the radio will play a jingle every 20 to 30 minutes. For this to work you need to have at least one song defined as Jingle.

**PAGINATE**

> Defines how many songs / artists that will be displayed per page.

**FORUM\_PAGINATE**

> Defines how many threads / posts that will be displayed per page.

**DOCUMENT\_ROOT**

> The file system path to the /static/ folder. Note: This should NOT be used on production systems!

**USE\_FULLTEXT\_SEARCH**

> This will turn on MATCH for song / artist search. Only for use on !MySQL, and Fulltext have to be manually enabled in the database.

**SHORTEN\_ONELINER\_LINKS**

> If set to 1, links in the OneLiner will be truncated. If the user pasted a link such as ![http://www.blah.com/blah/blah.jpg](http://www.blah.com/blah/blah.jpg) the oneliner would only show http://www.blah.com while still retaining a full link path when clicked. If turned off, the full link is also listed in the OneLiner. Default Value: 0

**MAX\_AVATAR\_SIZE**

> Allows you to specify the maximum size of an avatar, in bytes. The default value if not used is 60Kb.

**MAX\_AVATAR\_WIDTH**
**MAX\_AVATAR\_HEIGHT**

> Specify the maximum Width/Height of a user avatar in pixels. Default values for both are 100.

If you plan on using an external SMTP email server, you can adjust these settings to make the server use it:

**EMAIL\_HOST**

> This is the SMTP mail server name you want to connect to. Default is localhost (it assumes your server is handling mail processing).

**EMAIL\_PORT**

> The SMTP server port number. This should not normally change from 25, unless your ISP has some custom settings.

**EMAIL\_HOST\_USER**

> If your SMTP yuser requires authorization, put the username here. If the authentication is an email address, use the + character instead of @ in the email address.

**EMAIL\_HOST\_PASSWORD**

> Specify the password to your SMTP authorization. Leave blank if no authentication is needed.

**EMAIL\_USE\_TLS**

> Set to True or False if you need TLS for the server. Default is False.