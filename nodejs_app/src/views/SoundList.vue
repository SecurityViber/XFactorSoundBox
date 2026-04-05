<template>
  <div class="sound-list">
    <p v-if="sounds.length === 0" class="empty">
      No sounds found. Drop .mp3 files into src/assets/sounds/ and restart the dev server.
    </p>

    <ul v-else>
      <li v-for="(sound, index) in sounds" :key="sound" class="sound-item">
        <span class="sound-name">{{ fileName(sound) }}</span>
        <div class="controls">
          <button
            v-if="currentIndex !== index"
            class="btn btn-play"
            @click="play(index)"
          >
            ▶ Play
          </button>
          <button
            v-else
            class="btn btn-stop"
            @click="stop"
          >
            ■ Stop
          </button>
        </div>
      </li>
    </ul>
  </div>
</template>

<script setup>
import { ref, onUnmounted } from 'vue'
import { sounds } from '../sounds.js'

const currentIndex = ref(null)
let audioInstance = null

function fileName(path) {
  return path.split('/').pop()
}

function play(index) {
  stop()

  audioInstance = new Audio(sounds[index])
  currentIndex.value = index

  audioInstance.play().catch(() => {
    currentIndex.value = null
  })

  audioInstance.addEventListener('ended', () => {
    currentIndex.value = null
  })
}

function stop() {
  if (audioInstance) {
    audioInstance.pause()
    audioInstance.currentTime = 0
    audioInstance = null
  }
  currentIndex.value = null
}

onUnmounted(() => {
  stop()
})
</script>

<style scoped>
.sound-list {
  width: 100%;
  max-width: 600px;
  margin: 0 auto;
}

.empty {
  color: #888;
  text-align: center;
}

ul {
  list-style: none;
  display: flex;
  flex-direction: column;
  gap: 12px;
}

.sound-item {
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 16px 20px;
  background-color: #1a1a1a;
  border: 1px solid #2a2a2a;
  border-radius: 8px;
  transition: border-color 0.2s ease;
}

.sound-item:has(.btn-stop) {
  border-color: #f5c518;
}

.sound-name {
  color: #e0e0e0;
  font-size: 0.95rem;
  word-break: break-all;
}

.controls {
  flex-shrink: 0;
  margin-left: 16px;
}

.btn {
  padding: 8px 20px;
  font-size: 0.9rem;
  font-weight: 600;
  border: none;
  border-radius: 6px;
  cursor: pointer;
  transition: opacity 0.2s ease, transform 0.1s ease;
}

.btn:hover {
  opacity: 0.85;
  transform: translateY(-1px);
}

.btn:active {
  transform: translateY(0);
}

.btn-play {
  background-color: #f5c518;
  color: #0a0a0a;
}

.btn-stop {
  background-color: #ff4f4f;
  color: #fff;
}
</style>
