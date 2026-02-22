<template>
  <div class="container">
    <h1 class="title">XFactorSoundbox</h1>

    <nav class="nav">
      <RouterLink to="/" class="nav-link">Random Play</RouterLink>
      <RouterLink to="/sounds" class="nav-link">Sound List</RouterLink>
      <RouterLink to="/nico" class="nav-link">Nico Rules</RouterLink>
    </nav>

    <RouterView />
  </div>
</template>

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
  padding: 40px 20px;
}

.title {
  color: #f5c518;
  font-size: 3rem;
  font-weight: 800;
  letter-spacing: 2px;
  text-transform: uppercase;
  text-shadow: 0 0 20px rgba(245, 197, 24, 0.5);
}

.nav {
  display: flex;
  gap: 8px;
  background-color: #1a1a1a;
  padding: 6px;
  border-radius: 10px;
}

.nav-link {
  padding: 8px 24px;
  font-size: 0.95rem;
  font-weight: 600;
  color: #888;
  text-decoration: none;
  border-radius: 6px;
  transition: color 0.2s ease, background-color 0.2s ease;
}

.nav-link:hover {
  color: #e0e0e0;
}

.nav-link.router-link-active {
  background-color: #f5c518;
  color: #0a0a0a;
}
</style>
