- when you run a separate website server (e.g. run api-server on localhost and test website on localhost:9000) some
  images (links to device icons for example) wont display right - and not really a bug. We emit relative URLs
  which works fine in the real site, but not in the :9000 site.

  I suppose I COULD convert these relative URLs to absolute, and that might be better (would work in this case) but
  might cause trouble and doesnt appear worth the effort.