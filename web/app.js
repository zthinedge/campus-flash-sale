const state = {
  token: localStorage.getItem("campus_token") || "",
  user: JSON.parse(localStorage.getItem("campus_user") || "null"),
  products: [],
  activities: [],
  orders: [],
  admin: null,
  activeView: "overview"
};

const $ = (id) => document.getElementById(id);
const isAdmin = () => state.user?.role === "ADMIN";

function escapeHtml(value) {
  return String(value ?? "").replace(/[&<>"']/g, (char) => ({
    "&": "&amp;", "<": "&lt;", ">": "&gt;", '"': "&quot;", "'": "&#39;"
  })[char]);
}

function money(cents) {
  return `¥${(Number(cents || 0) / 100).toLocaleString("zh-CN", {
    minimumFractionDigits: 2,
    maximumFractionDigits: 2
  })}`;
}

function statusText(status) {
  return {
    ACTIVE: "进行中", PENDING: "未开始", ENDED: "已结束", CANCELED: "已取消",
    ON_SALE: "在售", IN_FLASH_SALE: "活动中", SOLD: "已售出", OFF_SHELF: "已下架",
    CREATED: "已创建"
  }[status] || status;
}

function toast(message, type = "info") {
  const element = $("toast");
  element.textContent = message;
  element.dataset.type = type;
  element.classList.add("show");
  clearTimeout(window.__toastTimer);
  window.__toastTimer = setTimeout(() => element.classList.remove("show"), 2800);
}

async function api(path, options = {}) {
  const headers = { "Content-Type": "application/json", ...(options.headers || {}) };
  if (state.token) headers.Authorization = `Bearer ${state.token}`;
  const response = await fetch(path, { ...options, headers });
  const data = await response.json().catch(() => ({ ok: false, error: "服务器返回了无效数据" }));
  if (!data.ok) {
    if (response.status === 401 && state.token) clearSession();
    throw new Error(data.error || `请求失败 (${response.status})`);
  }
  return data;
}

function saveSession(data) {
  state.token = data.token;
  state.user = data.user;
  localStorage.setItem("campus_token", state.token);
  localStorage.setItem("campus_user", JSON.stringify(state.user));
  renderSession();
}

function clearSession() {
  state.token = "";
  state.user = null;
  state.orders = [];
  state.admin = null;
  localStorage.removeItem("campus_token");
  localStorage.removeItem("campus_user");
  renderSession();
  renderOrders();
}

function renderSession() {
  const loggedIn = Boolean(state.user);
  $("currentUser").textContent = loggedIn ? state.user.username : "访客模式";
  $("currentRole").textContent = loggedIn ? (isAdmin() ? "系统管理员" : "普通学生用户") : "浏览公开活动";
  $("avatar").textContent = loggedIn ? state.user.username.slice(0, 1).toUpperCase() : "访";
  $("openAuthBtn").classList.toggle("hidden", loggedIn);
  $("logoutBtn").classList.toggle("hidden", !loggedIn);
  $("heroLoginBtn").classList.toggle("hidden", loggedIn);
  document.body.classList.toggle("is-authenticated", loggedIn);
  document.body.classList.toggle("is-admin", isAdmin());

  if (!loggedIn && ["orders", "admin"].includes(state.activeView)) switchView("overview");
  if (loggedIn && !isAdmin() && state.activeView === "admin") switchView("overview");
}

function openModal(id) {
  $(id).classList.remove("hidden");
}

function closeModal(id) {
  $(id).classList.add("hidden");
}

function switchView(view) {
  if (view === "orders" && !state.user) {
    openModal("authModal");
    return;
  }
  if (view === "admin" && !isAdmin()) {
    toast("该页面需要管理员权限", "error");
    return;
  }
  state.activeView = view;
  document.querySelectorAll(".view").forEach((item) => item.classList.toggle("active", item.id === `view-${view}`));
  document.querySelectorAll(".nav-item").forEach((item) => item.classList.toggle("active", item.dataset.view === view));
  $("pageTitle").textContent = {
    overview: "系统总览", market: "限时抢购", orders: "我的订单", admin: "管理看板"
  }[view];
}

function renderOverview() {
  const totalStock = state.activities.reduce((sum, item) => sum + item.availableStock, 0);
  const totalInitialStock = state.activities.reduce((sum, item) => sum + item.totalStock, 0);
  const metrics = [
    ["PRODUCT", "公开商品", state.products.length, "校园闲置商品"],
    ["ACTIVITY", "进行中活动", state.activities.length, "当前可参与"],
    ["STOCK", "剩余活动库存", totalStock, `初始库存 ${totalInitialStock}`],
    ["SAFETY", "一致性约束", "3 层", "事务 / 条件更新 / 唯一索引"]
  ];
  $("overviewMetrics").innerHTML = metrics.map(([code, label, value, hint]) => `
    <article class="metric">
      <span>${label}</span><strong>${value}</strong><small>${code}</small><p>${hint}</p>
    </article>
  `).join("");

  $("overviewActivities").innerHTML = state.activities.slice(0, 4).map((activity) => {
    const sold = activity.totalStock - activity.availableStock;
    const progress = Math.round((sold / Math.max(1, activity.totalStock)) * 100);
    return `
      <div class="activity-row">
        <div class="activity-index">${String(activity.id).padStart(2, "0")}</div>
        <div class="activity-main">
          <strong>${escapeHtml(activity.product.title)}</strong>
          <span>${escapeHtml(activity.activityNo)} · ${money(activity.flashPriceCents)}</span>
          <div class="stock-progress"><i style="width:${progress}%"></i></div>
        </div>
        <div class="activity-stock"><b>${activity.availableStock}</b><span>剩余</span></div>
      </div>`;
  }).join("") || `<div class="empty-state">当前没有进行中的活动</div>`;
}

function filteredActivities() {
  const keyword = $("marketSearch").value.trim().toLowerCase();
  const stock = $("stockFilter").value;
  return state.activities.filter((activity) => {
    const text = `${activity.activityNo} ${activity.product.title} ${activity.product.description}`.toLowerCase();
    const matchesStock = stock === "ALL"
      || (stock === "AVAILABLE" && activity.availableStock > 0)
      || (stock === "SOLD_OUT" && activity.availableStock === 0);
    return (!keyword || text.includes(keyword)) && matchesStock;
  });
}

function renderMarket() {
  $("activeCount").textContent = state.activities.length;
  $("saleGrid").innerHTML = filteredActivities().map((activity, index) => {
    const product = activity.product;
    const discount = Math.max(0, Math.round((1 - activity.flashPriceCents / product.originalPriceCents) * 100));
    const remaining = Math.round((activity.availableStock / Math.max(1, activity.totalStock)) * 100);
    return `
      <article class="sale-card">
        <div class="sale-cover cover-${index % 4}">
          <span class="sale-badge">限时 ${discount}% OFF</span>
          <div class="cover-code">${escapeHtml(product.productNo)}</div>
          <div class="cover-symbol">${["DB", "BIKE", "AUDIO", "KEY"][index % 4]}</div>
        </div>
        <div class="sale-content">
          <div class="sale-meta"><span>${escapeHtml(activity.activityNo)}</span><b>${statusText(activity.status)}</b></div>
          <h3>${escapeHtml(product.title)}</h3>
          <p>${escapeHtml(product.description)}</p>
          <div class="price-row"><strong>${money(activity.flashPriceCents)}</strong><del>${money(product.originalPriceCents)}</del></div>
          <div class="stock-line">
            <div><span>抢购进度</span><b>剩余 ${activity.availableStock} / ${activity.totalStock}</b></div>
            <div class="stock-progress"><i style="width:${100 - remaining}%"></i></div>
          </div>
          <button class="purchase-button" ${activity.availableStock === 0 ? "disabled" : ""}
            onclick="purchase(${activity.id})">${activity.availableStock === 0 ? "已售罄" : "立即抢购"}</button>
        </div>
      </article>`;
  }).join("") || `<div class="empty-state large">没有符合条件的活动</div>`;

  $("productGrid").innerHTML = state.products.map((product, index) => `
    <article class="product-card">
      <div class="product-number">${String(index + 1).padStart(2, "0")}</div>
      <span class="status-tag ${product.status.toLowerCase()}">${statusText(product.status)}</span>
      <h3>${escapeHtml(product.title)}</h3>
      <p>${escapeHtml(product.description)}</p>
      <div class="product-footer">
        <div><span>商品编号</span><code>${escapeHtml(product.productNo)}</code></div>
        <strong>${money(product.originalPriceCents)}</strong>
      </div>
    </article>
  `).join("") || `<div class="empty-state large">暂无商品</div>`;
}

function renderOrders() {
  $("orderCount").textContent = `${state.orders.length} 条交易记录`;
  $("ordersTable").innerHTML = state.orders.map((order) => `
    <tr>
      <td><code>${escapeHtml(order.orderNo)}</code></td>
      <td><strong>${escapeHtml(order.productTitle)}</strong><small>商品 ID ${order.productId}</small></td>
      <td>#${order.activityId}</td>
      <td><b>${money(order.totalAmountCents)}</b></td>
      <td><span class="status-pill">${statusText(order.status)}</span></td>
      <td>${escapeHtml(order.createdAt)}</td>
    </tr>
  `).join("") || `<tr><td colspan="6"><div class="empty-state">暂无订单，去抢购大厅看看吧</div></td></tr>`;
}

function renderAdmin() {
  if (!state.admin) return;
  const summary = state.admin.summary;
  const metrics = [
    ["活动总数", summary.activityCount, "ACTIVITY"],
    ["可用库存", summary.availableStock, "STOCK"],
    ["订单总数", summary.orderCount, "ORDER"],
    ["库存流水", summary.inventoryLogCount, "LOG"]
  ];
  $("adminMetrics").innerHTML = metrics.map(([label, value, code]) => `
    <article class="metric"><span>${label}</span><strong>${value}</strong><small>${code}</small></article>
  `).join("");

  $("adminTable").innerHTML = state.admin.activities.map((item) => {
    const activity = item.activity;
    const consistent = item.orderCount === item.inventoryLogCount;
    return `
      <tr>
        <td><strong>${escapeHtml(activity.activityNo)}</strong><small>#${activity.id}</small></td>
        <td>${escapeHtml(activity.product?.title || `商品 #${activity.productId}`)}</td>
        <td><b>${activity.availableStock}</b> / ${activity.totalStock}</td>
        <td>${item.orderCount}</td>
        <td>${item.inventoryLogCount}</td>
        <td><span class="consistency ${consistent ? "ok" : "bad"}">${consistent ? "一致" : "异常"}</span></td>
        <td><span class="status-tag ${activity.status.toLowerCase()}">${statusText(activity.status)}</span></td>
      </tr>`;
  }).join("") || `<tr><td colspan="7"><div class="empty-state">暂无活动数据</div></td></tr>`;
}

function fillActivityProducts() {
  $("activityProduct").innerHTML = state.products.map((product) =>
    `<option value="${product.id}">${escapeHtml(product.title)} · ${escapeHtml(product.productNo)}</option>`
  ).join("");
}

async function loadPublicData() {
  const [productsData, activitiesData] = await Promise.all([
    api("/api/products"),
    api("/api/activities")
  ]);
  state.products = productsData.products;
  state.activities = activitiesData.activities;
  fillActivityProducts();
  renderOverview();
  renderMarket();
}

async function loadOrders() {
  if (!state.user) return;
  state.orders = (await api("/api/orders")).orders;
  renderOrders();
}

async function loadAdmin() {
  if (!isAdmin()) return;
  state.admin = await api("/api/admin/summary");
  renderAdmin();
}

async function refreshAll(showToast = false) {
  try {
    await loadPublicData();
    const jobs = [];
    if (state.user) jobs.push(loadOrders());
    if (isAdmin()) jobs.push(loadAdmin());
    await Promise.all(jobs);
    if (showToast) toast("数据已刷新", "success");
  } catch (error) {
    toast(error.message, "error");
  }
}

async function authenticate(register) {
  try {
    const path = register ? "/api/register" : "/api/login";
    const data = await api(path, {
      method: "POST",
      body: JSON.stringify({
        username: $("username").value.trim(),
        password: $("password").value
      })
    });
    saveSession(data);
    closeModal("authModal");
    await refreshAll();
    toast(register ? "注册并登录成功" : "登录成功", "success");
  } catch (error) {
    toast(error.message, "error");
  }
}

async function purchase(activityId) {
  if (!state.user) {
    openModal("authModal");
    toast("请先登录后参与抢购");
    return;
  }
  try {
    const data = await api(`/api/activities/${activityId}/purchase`, {
      method: "POST",
      body: "{}"
    });
    await refreshAll();
    switchView("orders");
    toast(`抢购成功，订单 ${data.order.orderNo}`, "success");
  } catch (error) {
    toast(error.message, "error");
  }
}

async function publishProduct() {
  try {
    await api("/api/products", {
      method: "POST",
      body: JSON.stringify({
        productNo: $("productNo").value.trim(),
        title: $("productTitle").value.trim(),
        description: $("productDescription").value.trim(),
        originalPriceCents: Math.round(Number($("productPrice").value) * 100)
      })
    });
    closeModal("productModal");
    await refreshAll();
    toast("商品发布成功", "success");
  } catch (error) {
    toast(error.message, "error");
  }
}

async function createActivity() {
  try {
    await api("/api/activities", {
      method: "POST",
      body: JSON.stringify({
        activityNo: $("activityNo").value.trim(),
        productId: Number($("activityProduct").value),
        flashPriceCents: Math.round(Number($("flashPrice").value) * 100),
        totalStock: Number($("activityStock").value),
        startTime: $("startTime").value,
        endTime: $("endTime").value
      })
    });
    closeModal("activityModal");
    await refreshAll();
    toast("限时活动创建成功", "success");
  } catch (error) {
    toast(error.message, "error");
  }
}

function setDefaultTimes() {
  const now = new Date();
  const end = new Date(now.getTime() + 7 * 24 * 60 * 60 * 1000);
  const localValue = (date) => new Date(date.getTime() - date.getTimezoneOffset() * 60000).toISOString().slice(0, 16);
  $("startTime").value = localValue(now);
  $("endTime").value = localValue(end);
}

async function checkHealth() {
  try {
    await api("/api/health");
    $("dbStatus").textContent = "数据库已连接";
    $("dbDot").classList.add("ok");
  } catch (error) {
    $("dbStatus").textContent = "连接异常";
    toast(error.message, "error");
  }
}

async function restoreSession() {
  if (!state.token) return;
  try {
    state.user = (await api("/api/me")).user;
    localStorage.setItem("campus_user", JSON.stringify(state.user));
  } catch {
    clearSession();
  }
}

document.querySelectorAll("[data-view]").forEach((button) =>
  button.addEventListener("click", () => switchView(button.dataset.view)));
document.querySelectorAll("[data-jump]").forEach((button) =>
  button.addEventListener("click", () => switchView(button.dataset.jump)));
document.querySelectorAll("[data-close]").forEach((button) =>
  button.addEventListener("click", () => closeModal(button.dataset.close)));
document.querySelectorAll(".modal-backdrop").forEach((backdrop) =>
  backdrop.addEventListener("click", (event) => {
    if (event.target === backdrop) closeModal(backdrop.id);
  }));

$("openAuthBtn").addEventListener("click", () => openModal("authModal"));
$("heroLoginBtn").addEventListener("click", () => openModal("authModal"));
$("loginBtn").addEventListener("click", () => authenticate(false));
$("registerBtn").addEventListener("click", () => authenticate(true));
$("logoutBtn").addEventListener("click", async () => {
  try { await api("/api/logout", { method: "POST", body: "{}" }); } catch {}
  clearSession();
  await refreshAll();
  toast("已退出登录");
});
$("publishBtn").addEventListener("click", () => openModal("productModal"));
$("saveProductBtn").addEventListener("click", publishProduct);
$("createActivityBtn").addEventListener("click", () => {
  setDefaultTimes();
  openModal("activityModal");
});
$("saveActivityBtn").addEventListener("click", createActivity);
$("refreshBtn").addEventListener("click", () => refreshAll(true));
$("marketSearch").addEventListener("input", renderMarket);
$("stockFilter").addEventListener("change", renderMarket);

window.purchase = purchase;

setInterval(() => {
  $("clock").textContent = new Date().toLocaleString("zh-CN", { hour12: false });
}, 1000);

(async function init() {
  setDefaultTimes();
  await checkHealth();
  await restoreSession();
  renderSession();
  await refreshAll();
})();
