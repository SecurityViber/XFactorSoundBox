// Automatically picks up every .mp3 file in src/assets/sounds/
// Just drop files in that folder — no code changes needed.
const modules = import.meta.glob('./assets/sounds/*.mp3', {
  query: '?url',
  import: 'default',
  eager: true,
})

export const sounds = Object.values(modules)
