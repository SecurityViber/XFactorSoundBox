<template>
  <div class="container">
    <h1 class="title">XFactorSoundbox</h1>
    <button class="play-btn" @click="playRandom" :disabled="isPlaying">
      {{ isPlaying ? '▶ Playing...' : '▶ Play Sound' }}
    </button>
    <p v-if="error" class="error">{{ error }}</p>
  </div>
</template>

<script setup>
import { ref } from 'vue'

// List all your mp3 files here (placed in /public/sounds/)
const sounds = [
  '/sounds/sound1.mp3',
]

const isPlaying = ref(false)
const error = ref('')

function playRandom() {
  if (sounds.length === 0) {
    error.value = 'No sounds available. Add .mp3 files to /public/sounds/ and list them above.'
    return
  }

  error.value = ''
  const randomIndex = Math.floor(Math.random() * sounds.length)
  const audio = new Audio(sounds[randomIndex])

  isPlaying.value = true
  audio.play().catch(() => {
    error.value = 'Could not play sound. Make sure your .mp3 files are in /public/sounds/.'
    isPlaying.value = false
  })

  audio.addEventListener('ended', () => {
    isPlaying.value = false
  })
}
</script>

<style>
* {
  margin: 0;
  padding: 0;
  box-sizing: border-box;
}

body {
  background-color: #0a0a0a;
  font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
}

.container {
  min-height: 100vh;
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  gap: 48px;
}

.title {
  color: #f5c518;
  font-size: 3rem;
  font-weight: 800;
  letter-spacing: 2px;
  text-transform: uppercase;
  text-shadow: 0 0 20px rgba(245, 197, 24, 0.5);
}

.play-btn {
  padding: 20px 60px;
  font-size: 1.4rem;
  font-weight: 700;
  color: #0a0a0a;
  background-color: #f5c518;
  border: none;
  border-radius: 8px;
  cursor: pointer;
  transition: transform 0.1s ease, background-color 0.2s ease, box-shadow 0.2s ease;
  box-shadow: 0 4px 20px rgba(245, 197, 24, 0.4);
}

.play-btn:hover:not(:disabled) {
  background-color: #ffd700;
  box-shadow: 0 6px 28px rgba(245, 197, 24, 0.7);
  transform: translateY(-2px);
}

.play-btn:active:not(:disabled) {
  transform: translateY(0);
}

.play-btn:disabled {
  opacity: 0.6;
  cursor: not-allowed;
}

.error {
  color: #ff4f4f;
  font-size: 0.9rem;
  max-width: 400px;
  text-align: center;
}
</style>
