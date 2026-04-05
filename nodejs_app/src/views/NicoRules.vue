<template>
  <div class="nico-stage">
    <h2 class="nico-title">nico rules</h2>
  </div>
</template>

<style scoped>
.nico-stage {
  display: flex;
  align-items: center;
  justify-content: center;
  width: 300px;
  height: 300px;
}

.nico-title {
  color: #f5c518;
  font-size: 2rem;
  font-weight: 800;
  text-transform: uppercase;
  letter-spacing: 2px;
  text-shadow: 0 0 16px rgba(245, 197, 24, 0.6);
  animation: spin 3s linear infinite;
  transform-origin: center;
}

@keyframes spin {
  from { transform: rotate(0deg); }
  to   { transform: rotate(360deg); }
}
</style>
