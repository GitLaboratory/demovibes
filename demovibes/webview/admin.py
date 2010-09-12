from demovibes.webview.models import *
from django.contrib import admin
from django.contrib.contenttypes import generic

class SongLinkInline(generic.GenericTabularInline):
    qs_key = "S"
    extra = 1
    model = GenericLink
    def formfield_for_foreignkey(self, db_field, request, **kwargs):
        if db_field.name == "link":
            kwargs["queryset"] = GenericBaseLink.objects.filter(linktype = self.qs_key)
        return super(SongLinkInline, self).formfield_for_foreignkey(db_field, request, **kwargs)

class GroupLinkInline(SongLinkInline):
    qs_key = "G"

class UserLinkInline(SongLinkInline):
    qs_key = "U"

class LabelLinkInline(SongLinkInline):
    qs_key = "L"

class ArtistLinkInline(SongLinkInline):
    qs_key = "A"

class UserprofileAdmin(admin.ModelAdmin):
    search_fields = ['user']
    list_display = ['user', 'country', 'custom_css']
    inlines = [UserLinkInline]

class DownloadInline(admin.TabularInline):
    model = SongDownload
    extra = 3

class SongAdmin(admin.ModelAdmin):
    list_display = ['title', 'status', 'artist', 'uploader', 'bitrate', 'added', 'pouetid', 'explicit', 'info']
    list_editable = ['status']
    search_fields = ['title', 'status']
    list_filter = ['status']
    filter_horizontal = ['artists', 'groups', 'labels']
    fieldsets = [
        ("General"        ,{ 'fields' : ['title', 'release_year', 'remix_of_id', 'file', 'explicit', 'artists', 'groups', 'labels']}),
        ("Reference Info"    ,{ 'fields' : ['pouetid', 'dtv_id', 'wos_id', 'zxdemo_id', 'lemon_id', 'projecttwosix_id', 'hol_id', 'al_id', 'hvsc_url', 'type', 'platform', 'status', 'info']}),
        ("Technical Stuff"    ,{ 'fields' : ['song_length', 'bitrate','samplerate','replay_gain','loopfade_time']}),
    ]
    inlines = [DownloadInline, SongLinkInline]

class QueueAdmin(admin.ModelAdmin):
    list_display = ('song', 'requested', 'played', 'requested_by', 'priority', 'playtime')
    search_fields = ['song', 'requested', 'requested_by']
    list_filter = ['priority', 'played']
    fields = ['song', 'played', 'requested_by', 'priority', 'playtime']

class SongCommentAdmin(admin.ModelAdmin):
    list_display = ['song', 'user']

class GroupAdmin(admin.ModelAdmin):
    search_fields = ['name']
    inlines = [GroupLinkInline]

class NewsAdmin(admin.ModelAdmin):
    list_display = ('title', 'status', 'added')
    search_fields = ('title', 'text')

class ArtistAdmin(admin.ModelAdmin):
    search_fields = ('handle', 'name')
    list_display = ('handle', 'name', 'link_to_user')
    filter_horizontal = ['groups', 'labels']
    list_filter = ['status']
    fieldsets = [
        ("General info", {'fields' : ['handle', 'status', 'name', 'webpage', 'artist_pic', 'groups'] }),
        ("Personalia", {'fields' : ['dob', 'home_country', 'home_location', 'is_deceased', 'deceased_date', 'info'] }),
        ("NectaStuff", {'fields' : ['alias_of','created_by', 'link_to_user', 'labels' ] }),
        ("Other web pages", {'fields' : ['twitter_id', 'wiki_link', 'hol_id', 'last_fm_id'] }),
    ]
    inlines = [ArtistLinkInline]

class CompilationAdmin(admin.ModelAdmin):
    list_display = ('name', 'rel_date', 'date_added', 'created_by', 'status')
    search_fields = ['name'] # For now, we only need to search by the name of the compilation
    filter_horizontal = ['songs', 'prod_groups', 'prod_artists']
    list_filter = ['status']
    raw_id_fields = ["songs", "prod_artists", "prod_groups"]

class LabelAdmin(admin.ModelAdmin):
    search_fields =  ['name']
    inlines = [LabelLinkInline]
    list_display = ('name', 'found_date', 'last_updated', 'created_by')

class LinkAdmin(admin.ModelAdmin):
    search_fields = ('link_title', 'link_url') # Because we might want to find links to a specific site
    list_display = ('name', 'link_title', 'link_url', 'link_type', 'added', 'submitted_by', 'priority')

class FaqAdmin(admin.ModelAdmin):
    search_fields = ('question', 'answer')
    list_display = ('question', 'added', 'added_by', 'priority', 'answer', 'active')

class ScreenshotAdmin(admin.ModelAdmin):
    search_fields = ['name']
    list_display = ('name', 'last_updated', 'description', 'active')

class RadioStreamAdmin(admin.ModelAdmin):
    search_fields = ('name', 'user', 'country_code')
    list_display = ('name', 'bitrate', 'user', 'streamtype', 'country_code', 'active')
    list_editable = ['active']
    list_filter = ['active', 'streamtype']

class GBLAdmin(admin.ModelAdmin):
    list_display = ['name', 'linktype', 'link']
    list_filter = ['linktype']
    search_fields = ['name', 'link']

admin.site.register(Group, GroupAdmin)
admin.site.register(Song, SongAdmin)
admin.site.register(SongType)
admin.site.register(Theme)
admin.site.register(RadioStream, RadioStreamAdmin)
admin.site.register(News, NewsAdmin)
admin.site.register(Artist, ArtistAdmin)
admin.site.register(Userprofile, UserprofileAdmin)
admin.site.register(SongPlatform)
admin.site.register(Logo)
admin.site.register(GenericBaseLink, GBLAdmin)
admin.site.register(Queue, QueueAdmin)
admin.site.register(SongComment, SongCommentAdmin)
admin.site.register(Compilation, CompilationAdmin)
admin.site.register(Label, LabelAdmin)
admin.site.register(Link, LinkAdmin)
admin.site.register(LinkCategory)
admin.site.register(Faq, FaqAdmin)
admin.site.register(Screenshot, ScreenshotAdmin)
