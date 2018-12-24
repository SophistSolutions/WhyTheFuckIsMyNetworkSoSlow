// The Vue build version to load with the `import` command
// (runtime-only or standalone) has been set in webpack.base.conf with an alias.
import Vue from 'vue'
import App from './App'
import VueRouter from 'vue-router'

import Hello from './components/HelloWorld'
import About from './components/About'
import Param from './components/Param'
import ParamDetails from './components/ParamDetails'

Vue.use(VueRouter)

//define your routes
const routes = [
  //route for the home route of the web page
  { path: '/', component: Hello },
  //route for the about route of the web page
  { path: '/about', component: About },
  //route for the param route of the webpage
  { path: '/param', component: Param },
  //route for the paramdetails passing in params
  { path: '/paramdetails/:id', component: ParamDetails, name: 'ParamDetails' }
]

// Create the router instance and pass the `routes` option
// You can pass in additional options here, but let's
// keep it simple for now.
const router = new VueRouter({
  routes, // short for routes: routes
  mode: 'history'
})

//place the router guard
router.beforeEach((to, from, next) => {
    //check if the path user is going to is our param path
    if(to.path == '/param'){
        //check if the user item is already set
        if(localStorage.getItem('user')==undefined){
            //prompt for username
            var user = prompt('please enter your username');
            //prompt for password
            var pass = prompt('please enter your password');
            //check if th username and password given equals our preset details
            if (user == 'username' && pass == 'password'){
            //set the user item
            localStorage.setItem('user', user);
            //move to the route
            next();
            }else{
            //alert the username and pass is wrong
            alert('Wrong username and password, you do not have permission to access that route');
            //return, do not move to the route
            return;
            }

        }

    }

    next()
})

new Vue({
//define the selector for the root component
  el: '#app',
  //pass the template to the root component
  template: '<App/>',
  //declare components that the root component can access
  components: { App },
  //pass in the router to the Vue instance
  router
}).$mount('#app')//mount the router on the app
