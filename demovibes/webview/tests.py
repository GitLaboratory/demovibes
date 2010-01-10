from django.test import TestCase
from webview import models
from django.core.urlresolvers import reverse

# For documentation see:
#  Django specific : http://docs.djangoproject.com/en/1.1/topics/testing/#topics-testing
#  Python specific : http://docs.python.org/library/unittest.html

class BasicTest(TestCase):
    
    def setUp(self):
        """
        Run before each unit test, set up common values
        """
        pass
    
    def tearDown(self):
        """
        Run afer each unit test, cleanup
        """
        pass
    
    def testPagesLoad(self):
        """
        Basic check if public pages load or not
        """
        pages = ['dv-root', 'dv-songs', 'dv-platforms', "dv-streams", "dv-oneliner",
                 "dv-search", "dv-recent", "dv-groups", "dv-artists", "dv-compilations",
                 "dv-queue", "dv-labels", "dv-links"]
        for page in pages:
            r = self.client.get(reverse(page))
            self.failUnlessEqual(r.status_code, 200, "Failed to load page %s" % page)
    
    def testOneliner(self):
        """
        Test of oneliner
        """
        r = self.client.post(reverse("dv-oneliner_submit"), {'Line': "Test"})
        self.assertRedirects(r, reverse("auth_login")+"?next=%s" % reverse("dv-oneliner_submit"))
        user = models.User.objects.create_user('testuser', 'test@test.com', 'userpw')
        user.save()
        r = self.client.post(reverse("auth_login"), {'username': "testuser", 'password': "userpw"})
        self.assertNotEqual(r.status_code, 200)
        self.assertEqual(models.Oneliner.objects.count(), 0)
        r = self.client.post(reverse("dv-oneliner_submit"), {'Line': "Test"})
        self.assertEqual(models.Oneliner.objects.count(), 1)
        r = self.client.post(reverse("dv-oneliner_submit"), {'Line': ""})
        self.assertEqual(models.Oneliner.objects.count(), 1)
        r = self.client.get(reverse("dv-oneliner"))
        self.assertContains(r, "Test")
        self.assertContains(r, "testuser")