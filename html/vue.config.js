const path = require(`path`);

module.exports = {
  // "transpileDependencies": [
  //   "vuetify"
  // ],
  devServer: {
    port: 8081
  },
  pluginOptions: {
    vuetify: {
      // https://github.com/vuetifyjs/vuetify-loader/tree/next/packages/vuetify-loader
    }
  },
  // configureWebpack: {
  //   resolve: {
  //     // suggested by https://stackoverflow.com/questions/68293064/vue-3-invalid-vnode-type but didnt help
  //     symlinks: false,
  //     alias: {
  //       vue: path.resolve(`./node_modules/vue`),
  //       "@": path.resolve(__dirname, './src'),
  //       'vue$': 'vue/dist/vue.esm-bundler.js',
  //       // vue: path.resolve(__dirname, `../node_modules/vue`)
  //     }
  //   }
  // }
}
