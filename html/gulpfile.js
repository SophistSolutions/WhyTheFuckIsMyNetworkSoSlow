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
    return del([
        'dist/**/*'
    ]);
});

gulp.task('dependencies', function () {
    return gulp.src(paths.dependencies)
        .pipe(gulp.dest("dist/js"));
});

gulp.task('images', function () {
    return gulp.src(paths.images)
        .pipe(gulp.dest("dist/images"));
});

gulp.task("copy-html", function () {
    return gulp.src(paths.pages)
        .pipe(gulp.dest("dist"));
});

gulp.task("copy-styles", function () {
    return gulp.src(paths.styles)
        .pipe(gulp.dest("dist"));
});

gulp.task("copy-systemjs", function () {
    return gulp.src(paths.systemjs)
        .pipe(gulp.dest("dist"));
});

gulp.task("copy-mock-devices", function () {
    return gulp.src(paths.devices)
        .pipe(gulp.dest("dist"));
});

gulp.task('default', gulp.series('clean', gulp.parallel(["dependencies", "images", "copy-html","copy-styles", "copy-systemjs", "copy-mock-devices"], function () {
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
})));