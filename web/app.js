const state = {
  token: localStorage.getItem("campus_token") || "",
  user: JSON.parse(localStorage.getItem("campus_user") || "null"),
  products: [],
  activities: [],
  orders: [],
  admin: null,
  activeView: "overview",
  pendingActivityId: null
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

function formatDate(value) {
  if (!value) return "待定";
  const date = new Date(String(value).replace(" ", "T"));
  return Number.isNaN(date.getTime())
    ? value
    : date.toLocaleString("zh-CN", { month: "2-digit", day: "2-digit", hour: "2-digit", minute: "2-digit", hour12: false });
}

function remainingTime(endTime) {
  const end = new Date(String(endTime).replace(" ", "T")).getTime();
  const distance = end - Date.now();
  if (!Number.isFinite(end) || distance <= 0) return "活动已结束";
  const days = Math.floor(distance / 86400000);
  const hours = Math.max(1, Math.floor((distance % 86400000) / 3600000));
  return days > 0 ? `还剩 ${days} 天 ${hours} 小时` : `还剩 ${hours} 小时`;
}

function statusText(status) {
  return {
    ACTIVE: "抢购中", PENDING: "即将开始", ENDED: "已结束", CANCELED: "已取消",
    ON_SALE: "正在出售", IN_FLASH_SALE: "限时活动中", SOLD: "已售出", OFF_SHELF: "已下架",
    CREATED: "下单成功"
  }[status] || status;
}

function friendlyError(message) {
  const source = String(message || "");
  const translations = [
    ["username length must be between 3 and 64", "用户名需要填写 3 至 64 个字符"],
    ["password length must be between 4 and 128", "密码需要填写 4 至 128 个字符"],
    ["username already exists", "这个用户名已经被使用，请换一个试试"],
    ["invalid username or password", "用户名或密码不正确"],
    ["user account is disabled", "该账号暂时无法使用"],
    ["invalid product input", "请完整填写商品名称和价格"],
    ["administrator role is required", "只有管理员可以创建活动"],
    ["activity stock is sold out", "来晚一步，这场活动已经售罄"],
    ["activity is not available", "这场活动暂时不能参与"],
    ["user has already purchased this activity", "你已经抢购过这件商品了"],
    ["Duplicate entry", "提交的信息已存在，请修改后重试"]
  ];
  const match = translations.find(([key]) => source.includes(key));
  return match ? match[1] : source || "操作没有完成，请稍后重试";
}

function toast(message, type = "info") {
  const element = $("toast");
  element.textContent = friendlyError(message);
  element.dataset.type = type;
  element.classList.add("show");
  clearTimeout(window.__toastTimer);
  window.__toastTimer = setTimeout(() => element.classList.remove("show"), 3000);
}

async function api(path, options = {}) {
  const headers = { "Content-Type": "application/json", ...(options.headers || {}) };
  if (state.token) headers.Authorization = `Bearer ${state.token}`;

  let response;
  try {
    response = await fetch(path, { ...options, headers });
  } catch {
    throw new Error("暂时无法连接平台，请确认服务已经启动");
  }

  const data = await response.json().catch(() => ({ ok: false, error: "平台返回了无法识别的数据" }));
  if (!data.ok) {
    if (response.status === 401 && state.token) clearSession();
    throw new Error(friendlyError(data.error || `请求失败 (${response.status})`));
  }
  return data;
}

function setButtonBusy(button, busy, text = "处理中...") {
  if (!button) return;
  if (busy) {
    button.dataset.originalText = button.textContent;
    button.textContent = text;
    button.disabled = true;
    button.classList.add("is-loading");
  } else {
    button.textContent = button.dataset.originalText || button.textContent;
    button.disabled = false;
    button.classList.remove("is-loading");
  }
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
  $("currentUser").textContent = loggedIn ? state.user.username : "欢迎来到闪集";
  $("currentRole").textContent = loggedIn ? (isAdmin() ? "活动管理员" : "校园用户") : "登录后参与限时抢购";
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
  document.body.classList.add("modal-open");
}

function closeModal(id) {
  $(id).classList.add("hidden");
  if (![...document.querySelectorAll(".modal-backdrop")].some((item) => !item.classList.contains("hidden"))) {
    document.body.classList.remove("modal-open");
  }
}

function closeAllModals() {
  document.querySelectorAll(".modal-backdrop").forEach((item) => item.classList.add("hidden"));
  document.body.classList.remove("modal-open");
}

function switchView(view) {
  if (view === "orders" && !state.user) {
    openModal("authModal");
    toast("登录后才能查看个人订单");
    return;
  }
  if (view === "admin" && !isAdmin()) {
    toast("该页面仅对管理员开放", "error");
    return;
  }

  state.activeView = view;
  document.querySelectorAll(".view").forEach((item) => item.classList.toggle("active", item.id === `view-${view}`));
  document.querySelectorAll(".nav-item").forEach((item) => item.classList.toggle("active", item.dataset.view === view));
  $("pageTitle").textContent = {
    overview: "发现", market: "抢购大厅", orders: "我的订单", admin: "活动管理"
  }[view];
  window.scrollTo({ top: 0, behavior: "smooth" });
}

function renderOverview() {
  const totalStock = state.activities.reduce((sum, item) => sum + item.availableStock, 0);
  const initialStock = state.activities.reduce((sum, item) => sum + item.totalStock, 0);
  const soldCount = Math.max(0, initialStock - totalStock);
  const metrics = [
    ["物", "校园闲置", state.products.length, "件商品正在展示"],
    ["场", "限时活动", state.activities.length, "场活动可以参与"],
    ["余", "剩余数量", totalStock, "件好物等待新主人"],
    ["成", "已经抢到", soldCount, "件商品成功下单"]
  ];

  $("overviewMetrics").innerHTML = metrics.map(([mark, label, value, hint], index) => `
    <article class="metric metric-${index + 1}">
      <div class="metric-mark">${mark}</div>
      <span>${label}</span><strong>${value}</strong><p>${hint}</p>
    </article>
  `).join("");

  $("overviewActivities").innerHTML = state.activities.slice(0, 4).map((activity) => {
    const sold = activity.totalStock - activity.availableStock;
    const progress = Math.round((sold / Math.max(1, activity.totalStock)) * 100);
    return `
      <button class="activity-row activity-link" data-action="open-activity" data-id="${activity.id}">
        <div class="activity-index">${escapeHtml(activity.product.title.slice(0, 1))}</div>
        <div class="activity-main">
          <strong>${escapeHtml(activity.product.title)}</strong>
          <span>${money(activity.flashPriceCents)} · ${remainingTime(activity.endTime)}</span>
          <div class="stock-progress"><i style="width:${progress}%"></i></div>
        </div>
        <div class="activity-stock"><b>${activity.availableStock}</b><span>剩余</span></div>
        <span class="row-arrow">查看</span>
      </button>`;
  }).join("") || `
    <div class="empty-state">
      <strong>今天还没有限时活动</strong>
      <span>可以先去看看其他校园闲置</span>
      <button data-action="go-market">浏览商品</button>
    </div>`;
}

function filteredActivities() {
  const keyword = $("marketSearch").value.trim().toLowerCase();
  const stock = $("stockFilter").value;
  return state.activities.filter((activity) => {
    const text = `${activity.product.title} ${activity.product.description}`.toLowerCase();
    const matchesStock = stock === "ALL"
      || (stock === "AVAILABLE" && activity.availableStock > 0)
      || (stock === "SOLD_OUT" && activity.availableStock === 0);
    return (!keyword || text.includes(keyword)) && matchesStock;
  });
}

function filteredProducts() {
  const keyword = $("marketSearch").value.trim().toLowerCase();
  return state.products.filter((product) =>
    !keyword || `${product.title} ${product.description}`.toLowerCase().includes(keyword));
}

function renderMarket() {
  $("activeCount").textContent = state.activities.length;
  $("saleGrid").innerHTML = filteredActivities().map((activity, index) => {
    const product = activity.product;
    const discount = Math.max(0, Math.round((1 - activity.flashPriceCents / product.originalPriceCents) * 100));
    const soldPercent = Math.round(((activity.totalStock - activity.availableStock) / Math.max(1, activity.totalStock)) * 100);
    return `
      <article class="sale-card clickable-card" data-action="open-activity" data-id="${activity.id}">
        <div class="sale-cover cover-${index % 4}">
          <span class="sale-badge">限时优惠 ${discount}%</span>
          <div class="cover-code">${remainingTime(activity.endTime)}</div>
          <div class="cover-symbol">${escapeHtml(product.title.slice(0, 2))}</div>
        </div>
        <div class="sale-content">
          <div class="sale-meta"><span>校园闲置精选</span><b>${statusText(activity.status)}</b></div>
          <h3>${escapeHtml(product.title)}</h3>
          <p>${escapeHtml(product.description)}</p>
          <div class="price-row"><strong>${money(activity.flashPriceCents)}</strong><del>${money(product.originalPriceCents)}</del></div>
          <div class="stock-line">
            <div><span>已有 ${activity.totalStock - activity.availableStock} 人抢到</span><b>剩余 ${activity.availableStock} 件</b></div>
            <div class="stock-progress"><i style="width:${soldPercent}%"></i></div>
          </div>
          <div class="card-actions">
            <button class="outline-button" data-action="open-activity" data-id="${activity.id}">查看详情</button>
            <button class="purchase-button" data-action="begin-purchase" data-id="${activity.id}"
              ${activity.availableStock === 0 ? "disabled" : ""}>${activity.availableStock === 0 ? "已经售罄" : "立即抢购"}</button>
          </div>
        </div>
      </article>`;
  }).join("") || `
    <div class="empty-state large">
      <strong>没有找到符合条件的活动</strong>
      <span>换个关键词或者查看全部活动试试</span>
      <button data-action="clear-filter">清除筛选</button>
    </div>`;

  $("productGrid").innerHTML = filteredProducts().map((product, index) => {
    const activity = state.activities.find((item) => item.productId === product.id);
    return `
      <article class="product-card clickable-card" data-action="open-product" data-id="${product.id}">
        <div class="product-number">${String(index + 1).padStart(2, "0")}</div>
        <span class="status-tag ${product.status.toLowerCase()}">${statusText(product.status)}</span>
        <h3>${escapeHtml(product.title)}</h3>
        <p>${escapeHtml(product.description)}</p>
        <div class="product-footer">
          <div><span>${activity ? "限时价格" : "发布价格"}</span><strong>${money(activity?.flashPriceCents ?? product.originalPriceCents)}</strong></div>
          <button class="text-button" data-action="open-product" data-id="${product.id}">查看详情</button>
        </div>
      </article>`;
  }).join("") || `<div class="empty-state large"><strong>没有找到相关商品</strong><span>尝试搜索其他关键词</span></div>`;
}

function renderOrders() {
  $("orderCount").textContent = `${state.orders.length} 条交易记录`;
  $("ordersTable").innerHTML = state.orders.map((order) => `
    <tr>
      <td><code>${escapeHtml(order.orderNo)}</code></td>
      <td><strong>${escapeHtml(order.productTitle)}</strong><small>校园限时购</small></td>
      <td><b>${money(order.totalAmountCents)}</b></td>
      <td>${order.quantity} 件</td>
      <td><span class="status-pill">${statusText(order.status)}</span></td>
      <td>${formatDate(order.createdAt)}</td>
    </tr>
  `).join("") || `
    <tr><td colspan="6">
      <div class="empty-state order-empty">
        <strong>还没有订单</strong>
        <span>抢购成功的商品会出现在这里</span>
        <button data-action="go-market">去抢购大厅</button>
      </div>
    </td></tr>`;
}

function renderAdmin() {
  if (!state.admin) return;
  const summary = state.admin.summary;
  const metrics = [
    ["场", "全部活动", summary.activityCount, "已创建的限时活动"],
    ["余", "剩余商品", summary.availableStock, "当前可抢购数量"],
    ["单", "成交订单", summary.orderCount, "活动累计订单"],
    ["记", "库存记录", summary.inventoryLogCount, "库存变化记录"]
  ];
  $("adminMetrics").innerHTML = metrics.map(([mark, label, value, hint], index) => `
    <article class="metric metric-${index + 1}">
      <div class="metric-mark">${mark}</div><span>${label}</span><strong>${value}</strong><p>${hint}</p>
    </article>
  `).join("");

  $("adminTable").innerHTML = state.admin.activities.map((item) => {
    const activity = item.activity;
    const consistent = item.orderCount === item.inventoryLogCount;
    return `
      <tr class="clickable-row" data-action="open-admin-activity" data-id="${activity.id}">
        <td><strong>${escapeHtml(activity.product?.title || "校园商品")}</strong><small>创建于 ${formatDate(activity.createdAt)}</small></td>
        <td>${formatDate(activity.startTime)} 至 ${formatDate(activity.endTime)}</td>
        <td><b>${activity.availableStock}</b> / ${activity.totalStock}</td>
        <td>${item.orderCount}</td>
        <td>${item.inventoryLogCount}</td>
        <td><span class="consistency ${consistent ? "ok" : "bad"}">${consistent ? "记录正常" : "需要检查"}</span></td>
        <td><span class="status-tag ${activity.status.toLowerCase()}">${statusText(activity.status)}</span></td>
      </tr>`;
  }).join("") || `<tr><td colspan="7"><div class="empty-state"><strong>还没有活动</strong><span>发布商品后即可创建限时活动</span></div></td></tr>`;
}

function fillActivityProducts() {
  const eligible = state.products.filter((product) => product.status === "ON_SALE");
  $("activityProduct").innerHTML = eligible.length
    ? eligible.map((product) => `<option value="${product.id}">${escapeHtml(product.title)} · ${money(product.originalPriceCents)}</option>`).join("")
    : `<option value="">暂无可创建活动的商品</option>`;
  $("saveActivityBtn").disabled = eligible.length === 0;
  $("activityHint").textContent = eligible.length
    ? "抢购价建议低于商品发布价格。"
    : "请先发布一件新商品，再回来创建活动。";
  if (eligible.length) syncActivityPrice();
}

function showDetail(product, activity = null) {
  const index = Math.max(0, state.products.findIndex((item) => item.id === product.id));
  $("detailCover").className = `detail-cover cover-${index % 4}`;
  $("detailCoverText").textContent = product.title.slice(0, 2);
  $("detailTitle").textContent = product.title;
  $("detailDescription").textContent = product.description || "发布者暂未填写更多商品介绍。";
  $("detailProductStatus").textContent = statusText(product.status);
  $("detailStatus").textContent = activity ? statusText(activity.status) : statusText(product.status);
  $("detailStatus").className = `status-tag ${(activity?.status || product.status).toLowerCase()}`;
  $("detailTime").textContent = activity ? remainingTime(activity.endTime) : `发布于 ${formatDate(product.createdAt)}`;
  $("detailPrice").textContent = money(activity?.flashPriceCents ?? product.originalPriceCents);
  $("detailOriginalPrice").textContent = activity ? money(product.originalPriceCents) : "";
  $("detailOriginalPrice").classList.toggle("hidden", !activity);
  $("detailStock").textContent = activity ? `${activity.availableStock} 件` : "等待活动";

  const primary = $("detailPrimaryBtn");
  const canPurchase = activity?.status === "ACTIVE" && activity.availableStock > 0
    && state.activities.some((item) => item.id === activity.id);
  primary.dataset.activityId = canPurchase ? activity.id : "";
  primary.disabled = !canPurchase;
  primary.textContent = !activity
    ? "暂无限时活动"
    : activity.availableStock === 0
      ? "已经售罄"
      : canPurchase ? "立即抢购" : "当前不可参与";
  openModal("detailModal");
}

function openProductDetail(productId) {
  const product = state.products.find((item) => item.id === Number(productId));
  if (!product) return toast("没有找到这件商品", "error");
  const activity = state.activities.find((item) => item.productId === product.id) || null;
  showDetail(product, activity);
}

function openActivityDetail(activityId) {
  const activity = state.activities.find((item) => item.id === Number(activityId));
  if (!activity) {
    const adminActivity = state.admin?.activities.find((item) => item.activity.id === Number(activityId))?.activity;
    if (adminActivity?.product) return showDetail(adminActivity.product, adminActivity);
    return toast("没有找到这场活动", "error");
  }
  showDetail(activity.product, activity);
}

function beginPurchase(activityId) {
  const activity = state.activities.find((item) => item.id === Number(activityId));
  if (!activity) return toast("这场活动暂时无法参与", "error");
  if (!state.user) {
    closeModal("detailModal");
    openModal("authModal");
    toast("登录后就可以参与抢购");
    return;
  }
  if (activity.availableStock === 0) return toast("来晚一步，这件商品已经售罄", "error");

  state.pendingActivityId = activity.id;
  $("purchaseProduct").textContent = activity.product.title;
  $("purchasePrice").textContent = money(activity.flashPriceCents);
  $("purchaseStock").textContent = `${activity.availableStock} 件`;
  closeModal("detailModal");
  openModal("purchaseModal");
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
    if (showToast) toast("页面内容已更新", "success");
  } catch (error) {
    toast(error.message, "error");
  }
}

function validateCredentials() {
  const username = $("username").value.trim();
  const password = $("password").value;
  if (username.length < 3) throw new Error("用户名至少需要 3 个字符");
  if (password.length < 4) throw new Error("密码至少需要 4 个字符");
  return { username, password };
}

async function authenticate(register, button) {
  try {
    const credentials = validateCredentials();
    setButtonBusy(button, true, register ? "正在注册..." : "正在登录...");
    const data = await api(register ? "/api/register" : "/api/login", {
      method: "POST",
      body: JSON.stringify(credentials)
    });
    saveSession(data);
    closeModal("authModal");
    await refreshAll();
    toast(register ? "注册成功，欢迎来到闪集" : `欢迎回来，${data.user.username}`, "success");
  } catch (error) {
    toast(error.message, "error");
  } finally {
    setButtonBusy(button, false);
  }
}

async function confirmPurchase() {
  const button = $("confirmPurchaseBtn");
  const activityId = state.pendingActivityId;
  if (!activityId) return;

  try {
    setButtonBusy(button, true, "正在为你抢购...");
    const data = await api(`/api/activities/${activityId}/purchase`, {
      method: "POST",
      body: "{}"
    });
    closeModal("purchaseModal");
    state.pendingActivityId = null;
    $("successOrderNo").textContent = data.order.orderNo;
    await refreshAll();
    openModal("successModal");
  } catch (error) {
    toast(error.message, "error");
    await refreshAll();
    closeModal("purchaseModal");
  } finally {
    setButtonBusy(button, false);
  }
}

function generateBusinessNo(prefix) {
  const now = new Date();
  const date = now.toISOString().slice(2, 10).replaceAll("-", "");
  const random = String(Math.floor(Math.random() * 9000) + 1000);
  return `${prefix}-${date}-${random}`;
}

async function publishProduct() {
  const button = $("saveProductBtn");
  const title = $("productTitle").value.trim();
  const description = $("productDescription").value.trim();
  const price = Number($("productPrice").value);

  if (!title) return toast("请填写商品名称", "error");
  if (!Number.isFinite(price) || price <= 0) return toast("请填写正确的商品价格", "error");
  if (!description) return toast("请简单描述商品成色和使用情况", "error");

  try {
    setButtonBusy(button, true, "正在发布...");
    await api("/api/products", {
      method: "POST",
      body: JSON.stringify({
        productNo: generateBusinessNo("PROD"),
        title,
        description,
        originalPriceCents: Math.round(price * 100)
      })
    });
    closeModal("productModal");
    $("productTitle").value = "";
    $("productPrice").value = "";
    $("productDescription").value = "";
    await refreshAll();
    switchView("market");
    toast("商品发布成功，管理员可以为它创建活动", "success");
  } catch (error) {
    toast(error.message, "error");
  } finally {
    setButtonBusy(button, false);
    fillActivityProducts();
  }
}

function syncActivityPrice() {
  const product = state.products.find((item) => item.id === Number($("activityProduct").value));
  if (product) $("flashPrice").value = (product.originalPriceCents * 0.8 / 100).toFixed(2);
}

async function createActivity() {
  const button = $("saveActivityBtn");
  const productId = Number($("activityProduct").value);
  const product = state.products.find((item) => item.id === productId);
  const flashPrice = Number($("flashPrice").value);
  const stock = Number($("activityStock").value);
  const startTime = $("startTime").value;
  const endTime = $("endTime").value;

  if (!product) return toast("请先选择一件商品", "error");
  if (!Number.isFinite(flashPrice) || flashPrice <= 0) return toast("请填写正确的抢购价格", "error");
  if (flashPrice * 100 > product.originalPriceCents) return toast("抢购价不能高于商品发布价格", "error");
  if (!Number.isInteger(stock) || stock < 1) return toast("活动数量至少为 1 件", "error");
  if (!startTime || !endTime || new Date(startTime) >= new Date(endTime)) return toast("结束时间需要晚于开始时间", "error");

  try {
    setButtonBusy(button, true, "正在创建...");
    await api("/api/activities", {
      method: "POST",
      body: JSON.stringify({
        activityNo: generateBusinessNo("ACT"),
        productId,
        flashPriceCents: Math.round(flashPrice * 100),
        totalStock: stock,
        startTime,
        endTime
      })
    });
    closeModal("activityModal");
    await refreshAll();
    switchView("admin");
    toast("新活动已经创建", "success");
  } catch (error) {
    toast(error.message, "error");
  } finally {
    setButtonBusy(button, false);
    fillActivityProducts();
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
    $("serviceStatus").textContent = "运行正常";
    $("serviceDot").classList.add("ok");
  } catch (error) {
    $("serviceStatus").textContent = "暂时不可用";
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
    if (event.target === backdrop && backdrop.id !== "successModal") closeModal(backdrop.id);
  }));

document.addEventListener("click", (event) => {
  const target = event.target.closest("[data-action]");
  if (!target || target.disabled) return;
  const action = target.dataset.action;
  const id = Number(target.dataset.id);

  if (action === "open-product") openProductDetail(id);
  if (action === "open-activity" || action === "open-admin-activity") openActivityDetail(id);
  if (action === "begin-purchase") beginPurchase(id);
  if (action === "go-market") switchView("market");
  if (action === "go-orders") {
    closeModal("successModal");
    switchView("orders");
  }
  if (action === "continue-shopping") {
    closeModal("successModal");
    switchView("market");
  }
  if (action === "clear-filter") {
    $("marketSearch").value = "";
    $("stockFilter").value = "ALL";
    renderMarket();
  }
  if (action === "use-demo") {
    $("username").value = target.dataset.username;
    $("password").value = target.dataset.password;
    $("username").focus();
    toast("演示账号已填入");
  }
});

$("openAuthBtn").addEventListener("click", () => openModal("authModal"));
$("heroLoginBtn").addEventListener("click", () => openModal("authModal"));
$("loginBtn").addEventListener("click", () => authenticate(false, $("loginBtn")));
$("registerBtn").addEventListener("click", () => authenticate(true, $("registerBtn")));
$("password").addEventListener("keydown", (event) => {
  if (event.key === "Enter") authenticate(false, $("loginBtn"));
});
$("logoutBtn").addEventListener("click", async () => {
  try { await api("/api/logout", { method: "POST", body: "{}" }); } catch {}
  clearSession();
  await refreshAll();
  switchView("overview");
  toast("已经安全退出");
});
$("publishBtn").addEventListener("click", () => openModal("productModal"));
$("saveProductBtn").addEventListener("click", publishProduct);
$("createActivityBtn").addEventListener("click", () => {
  setDefaultTimes();
  fillActivityProducts();
  openModal("activityModal");
});
$("activityProduct").addEventListener("change", syncActivityPrice);
$("saveActivityBtn").addEventListener("click", createActivity);
$("detailPrimaryBtn").addEventListener("click", () => beginPurchase(Number($("detailPrimaryBtn").dataset.activityId)));
$("confirmPurchaseBtn").addEventListener("click", confirmPurchase);
$("refreshBtn").addEventListener("click", async () => {
  setButtonBusy($("refreshBtn"), true, "刷新中...");
  await refreshAll(true);
  setButtonBusy($("refreshBtn"), false);
});
$("marketSearch").addEventListener("input", renderMarket);
$("stockFilter").addEventListener("change", renderMarket);

document.addEventListener("keydown", (event) => {
  if (event.key === "Escape") closeAllModals();
});

function updateClock() {
  $("clock").textContent = new Date().toLocaleDateString("zh-CN", {
    month: "long", day: "numeric", weekday: "short"
  });
}

(async function init() {
  updateClock();
  setDefaultTimes();
  await checkHealth();
  await restoreSession();
  renderSession();
  await refreshAll();
})();
