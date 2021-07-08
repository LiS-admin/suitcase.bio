var cacheName = "v1";
self.addEventListener('install', function (event) {
    event.waitUntil(
        caches.open(cacheName).then(function (cache) {
            return cache.addAll(
                [
                    'css/modal.css',
                    'css/style.css',
                    'js/modal.js',
                    'js/main.js',
                    'js/jquery-3.5.1.min.js',
                    'js/marvinj-1.0.min.js',
                    'index.html'

                ]
            );
        })
    );
});

self.addEventListener("fetch", function (e) {
    if (new URL(e.request.url).origin !== location.origin) return;

    if (e.request.mode === "navigate" && navigator.onLine) {
        e.respondWith(
            fetch(e.request).then(function (response) {
                return caches.open(cacheName).then(function (cache) {
                    cache.put(e.request, response.clone());
                    return response;
                });
            })
        );
        return;
    }

    e.respondWith(
        caches
            .match(e.request)
            .then(function (response) {
                return (
                    response ||
                    fetch(e.request).then(function (response) {
                        return caches.open(cacheName).then(function (cache) {
                            cache.put(e.request, response.clone());
                            return response;
                        });
                    })
                );
            })
            .catch(function () {
                return caches.match(offlinePage);
            })
    );
});

self.addEventListener('beforeinstallprompt', (event) => {
    console.log('👍', 'beforeinstallprompt', event);
    // Stash the event so it can be triggered later.
    window.deferredPrompt = event;
    // Remove the 'hidden' class from the install button container
    divInstall.classList.toggle('hidden', false);
});