import { createRouter, createWebHashHistory } from 'vue-router'
import Home from '../views/Home.vue'
import SoundList from '../views/SoundList.vue'
import NicoRules from '../views/NicoRules.vue'

const routes = [
  { path: '/', component: Home },
  { path: '/sounds', component: SoundList },
  { path: '/nico', component: NicoRules },
]

export default createRouter({
  history: createWebHashHistory(),
  routes,
})
