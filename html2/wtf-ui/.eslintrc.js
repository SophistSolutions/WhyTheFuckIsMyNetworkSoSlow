module.exports = {
  root: true,
  env: {
    node: true
  },
  'extends': [
    'plugin:vue/vue3-essential',
    'eslint:recommended',
    '@vue/typescript/recommended'
  ],
  parserOptions: {
    ecmaVersion: 2020
  },
  rules: {
    'no-console': process.env.NODE_ENV === 'production' ? 'warn' : 'off',
    'no-debugger': process.env.NODE_ENV === 'production' ? 'warn' : 'off',
    "@typescript-eslint/no-inferrable-types": 'off',
    "vue/multi-word-component-names": "off",
    "vue/no-deprecated-slot-scope-attribute" : "off",
    "vue/no-deprecated-slot-attribute": "off",
    "vue/no-deprecated-filter" : "off",
    "vue/no-deprecated-v-bind-sync": "off"
  }
}
