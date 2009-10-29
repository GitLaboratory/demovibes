from demovibes.webview.models import *
from django.contrib import admin

class UserprofileAdmin(admin.ModelAdmin):
	search_fields = ['user']
	list_display = ['user','country']

class DownloadInline(admin.TabularInline):
	model = SongDownload
	extra = 3

class SongAdmin(admin.ModelAdmin):
	list_display = ['title', 'status', 'artist', 'uploader', 'bitrate', 'added', 'pouetid', 'info']
	search_fields = ['title', 'status']
	list_filter = ['status']
	filter_horizontal = ['artists', 'groups', 'labels']
	fieldsets = [
		("General"		,{ 'fields' : ['title', 'release_date', 'remix_of_id', 'file', 'artists', 'groups', 'labels']}),
		("Additional info"	,{ 'fields' : ['pouetid', 'wos_id', 'zxdemo_id', 'lemon_id', 'projecttwosix_id', 'hol_id', 'hvsc_url', 'type', 'platform', 'status', 'info']}),
		("Technical stuff"	,{ 'fields' : ['song_length', 'bitrate','samplerate']}),
	]
	inlines = [DownloadInline]

class QueueAdmin(admin.ModelAdmin):
	list_display = ('song', 'requested', 'played', 'requested_by', 'priority', 'playtime')
	search_fields = ['song', 'requested', 'requested_by']
	list_filter = ['priority', 'played']
	fields = ['song', 'played', 'requested_by', 'priority', 'playtime']

class SongCommentAdmin(admin.ModelAdmin):
	list_display = ['song', 'user']

class GroupAdmin(admin.ModelAdmin):
	search_fields = ['name']

class NewsAdmin(admin.ModelAdmin):
	list_display = ('title', 'status', 'added')
	search_fields = ('title', 'text')

class ArtistAdmin(admin.ModelAdmin):
	search_fields = ('handle', 'name')
	list_display = ('handle', 'name', 'link_to_user')
	filter_horizontal = ['groups', 'labels']
	
class CompilationAdmin(admin.ModelAdmin):
	list_display = ('name', 'rel_date', 'date_added', 'created_by', 'status')
	search_fields = ['name'] # For now, we only need to search by the name of the compilation
	filter_horizontal = ['songs', 'prod_groups', 'prod_artists']
	
class LabelAdmin(admin.ModelAdmin):
	search_fields =  ['name']
	list_display = ('name', 'found_date', 'last_updated', 'created_by')

admin.site.register(Group, GroupAdmin)
admin.site.register(Song, SongAdmin)
admin.site.register(SongType)
admin.site.register(Theme)
admin.site.register(RadioStream)
admin.site.register(News, NewsAdmin)
admin.site.register(Artist, ArtistAdmin)
admin.site.register(Userprofile, UserprofileAdmin)
admin.site.register(SongPlatform)
admin.site.register(Logo)
admin.site.register(Queue, QueueAdmin)
admin.site.register(SongComment, SongCommentAdmin)
admin.site.register(Compilation, CompilationAdmin)
admin.site.register(Label, LabelAdmin)
