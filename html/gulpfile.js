var gulp = require("gulp");
var del = require("del");
var browserify = require("browserify");
var source = require('vinyl-source-stream');
var tsify = require("tsify");
var paths = {
    pages:        ['src/*.html'],
    styles:       ['src/styles.css'],
    systemjs:     ['src/systemjs.config.extras.js',
                   'src/systemjs.config.js'],
    devices:      ['src/devices.json'],
    dependencies: ['node_modules/core-js/client/shim.min.js',
                   'node_modules/zone.js/dist/zone.js',
                   'node_modules/systemjs/dist/system.src.js'],
    images:       ['src/images/*.png']
};

gulp.task('clean', function () {
    return del(['dist']);
});


gulp.task('build', ["dependencies", "images", "copy-html","copy-styles", "copy-systemjs", "copy-mock-devices"], function() {});


gulp.task('dependencies', ['clean'], function () {
    var stream = gulp.src(paths.dependencies)
        .pipe(gulp.dest("dist/js"));
    return stream;
});

gulp.task('images', ['clean'], function () {
    var stream = gulp.src(paths.images)
        .pipe(gulp.dest("dist/images"));
    return stream;
});

gulp.task("copy-html", ['clean'], function () {
    var stream = gulp.src(paths.pages)
        .pipe(gulp.dest("dist"));
    return stream;
});

gulp.task("copy-styles", ['clean'], function () {
    var stream = gulp.src(paths.styles)
        .pipe(gulp.dest("dist"));
    return stream;
});

gulp.task("copy-systemjs", ['clean'], function () {
    var stream = gulp.src(paths.systemjs)
        .pipe(gulp.dest("dist"));
    return stream;
});

gulp.task("copy-mock-devices", ['clean'], function () {
    var stream = gulp.src(paths.devices)
        .pipe(gulp.dest("dist"));
    return stream;
});

gulp.task('default', ['build'], function () {
    return browserify({
        basedir: '.',
        debug: true,
        entries: ['src/main.ts'],
        cache: {},
        packageCache: {}
    })
    .plugin(tsify)
    .bundle()
    .pipe(source('bundle.js'))
    .pipe(gulp.dest("dist"));
});