document.addEventListener('DOMContentLoaded', () => {
    // 全局WebSocket实例
    let websocket;

    // --- DOM 元素选择 ---
    const navLinks = document.querySelectorAll('.nav-link[data-page]');
    const pages = document.querySelectorAll('.page-content');
    
    // 仪表盘页面元素
    const dashboardUptime = document.getElementById('dashboard-uptime');
    const dashboardHeap = document.getElementById('dashboard-heap');
    const dashboardPsram = document.getElementById('dashboard-psram');
    const dashboardWifiStatus = document.getElementById('dashboard-wifi-status');
    const dashboardIpAddress = document.getElementById('dashboard-ip-address');
    const sysChipModel = document.getElementById('sys-chip-model');
    const sysCpuFreq = document.getElementById('sys-cpu-freq');
    const sysFlashSize = document.getElementById('sys-flash-size');
    const sysPsramSize = document.getElementById('sys-psram-size');
    const sysIdfVersion = document.getElementById('sys-idf-version');
    const logConsole = document.getElementById('log-console');
    const connectionStatusBadge = document.getElementById('connection-status');

    // 确认模态框元素
    const confirmationModalEl = document.getElementById('confirmationModal');
    const confirmationModal = new bootstrap.Modal(confirmationModalEl);
    const confirmationModalBody = document.getElementById('confirmationModalBody');
    const confirmActionBtn = document.getElementById('confirm-action-btn');
    let currentAction = null; // 用于存储当前待确认的操作


    // WiFi设置页面元素
    const wifiForm = document.getElementById('wifi-form');
    const wifiModeSelect = document.getElementById('wifi-mode-select');
    const scanWifiBtn = document.getElementById('scan-wifi-btn');
    const wifiScanResultsBody = document.getElementById('wifi-scan-results');
    const wifiSsidInput = document.getElementById('wifi-ssid');
    const wifiPasswordInput = document.getElementById('wifi-password');
    const togglePasswordVisibilityBtn = document.getElementById('toggle-password-visibility');
    const passwordToggleIcon = document.getElementById('password-toggle-icon');
    const wifiScanModal = new bootstrap.Modal(document.getElementById('wifi-scan-modal'));

    // --- 导航逻辑 ---
    navLinks.forEach(link => {
        link.addEventListener('click', (e) => {
            e.preventDefault();
            const pageId = `page-${link.dataset.page}`;

            // 切换页面显示
            pages.forEach(page => {
                page.style.display = page.id === pageId ? 'block' : 'none';
            });

            // 更新导航链接的激活状态
            navLinks.forEach(nav => nav.classList.remove('active'));
            link.classList.add('active');

            // 如果切换到设置页面，自动加载设置
            if (link.dataset.page === 'settings') {
                sendCommand({ command: 'get_wifi_settings' });
            } else if (link.dataset.page === 'dashboard') {
                // 如果切换到仪表盘页面，自动获取系统状态
                sendCommand({ command: 'get_system_status' });
            }

            // 在移动端视图下，点击链接后自动收起菜单
            const offcanvasElement = document.getElementById('offcanvasSidebar');
            const offcanvasSidebar = bootstrap.Offcanvas.getInstance(offcanvasElement);
            if (offcanvasSidebar && offcanvasElement.classList.contains('show')) {
                offcanvasSidebar.hide();
            }
        });
    });

    // --- WebSocket 通信 ---
    function initWebSocket() {
        const wsUrl = `ws://${window.location.hostname}/ws`;
        console.log(`Connecting to WebSocket at: ${wsUrl}`);
        websocket = new WebSocket(wsUrl);

        websocket.onopen = () => {
            console.log('WebSocket connection opened.');
            showToast('WebSocket 已连接', 'success');
            if (connectionStatusBadge) {
                connectionStatusBadge.classList.remove('bg-danger');
                connectionStatusBadge.classList.add('bg-success');
                connectionStatusBadge.textContent = '在线';
            }
            // 连接成功后，立即请求系统状态和WiFi设置，更新UI
            sendCommand({ command: 'get_system_status' });
            sendCommand({ command: 'get_wifi_settings' });
        };

        websocket.onclose = () => {
            console.log('WebSocket connection closed.');
            showToast('WebSocket 已断开，5秒后尝试重连...', 'danger');
            if (connectionStatusBadge) {
                connectionStatusBadge.classList.remove('bg-success');
                connectionStatusBadge.classList.add('bg-danger');
                connectionStatusBadge.textContent = '离线';
            }
            setTimeout(initWebSocket, 5000);
        };

        websocket.onerror = (error) => {
            console.error('WebSocket error observed:', error);
            showToast('WebSocket 连接错误', 'danger');
        };

        websocket.onmessage = (event) => {
            console.log('WebSocket message received:', event.data);
            try {
                const message = JSON.parse(event.data);
                handleWebSocketMessage(message);
            } catch (e) {
                console.error('Error parsing JSON:', e);
            }
        };
    }

    function sendCommand(command) {
        if (websocket && websocket.readyState === WebSocket.OPEN) {
            const commandString = JSON.stringify(command);
            websocket.send(commandString);
            console.log('Sent command string:', commandString);
        } else {
            showToast('WebSocket 未连接，无法发送命令。', 'warning');
        }
    }

    function handleWebSocketMessage(message) {
        switch (message.type) {
            case 'system_status':
                updateSystemStatusUI(message.data);
                break;
            case 'wifi_settings':
                updateWifiForm(message.data);
                break;
            case 'wifi_scan_result':
                updateWifiScanResults(message.data);
                break;
            case 'toast':
                showToast(message.data.message, message.data.level);
                break;
            case 'log': // 处理后端推送的日志
                appendLog(message.data.message, message.data.level);
                break;
            default:
                console.warn('Unknown message type received:', message.type);
        }
    }

    // --- UI 更新函数 ---
    const formatBytes = (bytes) => {
        if (bytes === undefined) return 'N/A';
        if (bytes > 1024 * 1024) return `${(bytes / (1024 * 1024)).toFixed(2)} MB`;
        return `${(bytes / 1024).toFixed(2)} KB`;
    };

    function updateSystemStatusUI(data) {
        if (dashboardUptime) dashboardUptime.textContent = `${data.uptime} 秒`;
        if (dashboardHeap) dashboardHeap.textContent = `${formatBytes(data.heap_free)} / ${formatBytes(data.heap_total)}`;
        if (dashboardPsram) dashboardPsram.textContent = `${formatBytes(data.psram_pool_free)} / ${formatBytes(data.psram_pool_total)}`;
        
        if (dashboardWifiStatus) {
            dashboardWifiStatus.textContent = data.wifi_connected ? '已连接' : '未连接';
            dashboardWifiStatus.parentElement.parentElement.classList.remove('bg-success', 'bg-danger');
            dashboardWifiStatus.parentElement.parentElement.classList.add(data.wifi_connected ? 'bg-success' : 'bg-danger');
        }
        if (dashboardIpAddress) dashboardIpAddress.textContent = data.wifi_ip || 'N/A';

        // 更新系统信息列表
        if (sysChipModel) sysChipModel.textContent = data.chip_model || 'N/A';
        if (sysCpuFreq) sysCpuFreq.textContent = `${data.cpu_freq || 'N/A'} MHz`;
        if (sysFlashSize) sysFlashSize.textContent = `${formatBytes(data.flash_size_bytes) || 'N/A'}`;
        if (sysPsramSize) sysPsramSize.textContent = `${formatBytes(data.psram_size_bytes) || 'N/A'}`;
        if (sysIdfVersion) sysIdfVersion.textContent = data.idf_version || 'N/A';
    }

    function updateWifiForm(data) {
        wifiSsidInput.value = data.ssid || '';
        wifiPasswordInput.value = data.password || ''; // 密码也回显，但实际应用中不建议回显明文密码
        if (wifiModeSelect) {
            wifiModeSelect.value = data.mode || 3; // 默认为AP+STA
            // 触发change事件以更新UI显示
            wifiModeSelect.dispatchEvent(new Event('change'));
        }

        // 更新静态IP设置
        document.getElementById('static-ip-toggle').checked = data.static_ip;
        document.getElementById('static-ip-address').value = data.ip || '';
        document.getElementById('static-ip-subnet').value = data.subnet || '';
        document.getElementById('static-ip-gateway').value = data.gateway || '';
        toggleStaticIpFields(data.static_ip); // 根据设置显示/隐藏静态IP字段
    }

    function updateWifiScanResults(networks) {
        wifiScanResultsBody.innerHTML = ''; // 清空旧结果
        if (networks.length === 0) {
            wifiScanResultsBody.innerHTML = '<tr><td colspan="3" class="text-center">未扫描到网络</td></tr>';
            return;
        }
        networks.forEach(net => {
            const row = document.createElement('tr');
            row.style.cursor = 'pointer';
            row.innerHTML = `<td>${net.ssid}</td><td>${net.rssi}</td><td>${getEncryptionType(net.encryption)}</td>`;
            row.addEventListener('click', () => {
                wifiSsidInput.value = net.ssid;
                wifiScanModal.hide();
            });
            wifiScanResultsBody.appendChild(row);
        });
    }
    
    function getEncryptionType(enc) {
        const types = { 5: 'WEP', 2: 'WPA_PSK', 4: 'WPA2_PSK', 6: 'WPA_WPA2_PSK', 7: 'WPA2_ENTERPRISE', 8: 'WPA3_PSK', 9: 'WPA2_WPA3_PSK', 0: 'OPEN' };
        return types[enc] || 'Unknown';
    }

    function showToast(message, level = 'info') {
        const toastContainer = document.createElement('div');
        toastContainer.className = `toast align-items-center text-white bg-${level} border-0 position-fixed bottom-0 end-0 m-3`;
        toastContainer.setAttribute('role', 'alert');
        toastContainer.setAttribute('aria-live', 'assertive');
        toastContainer.setAttribute('aria-atomic', 'true');
        toastContainer.innerHTML = `<div class="d-flex"><div class="toast-body">${message}</div><button type="button" class="btn-close btn-close-white me-2 m-auto" data-bs-dismiss="toast" aria-label="Close"></button></div>`;
        document.body.appendChild(toastContainer);
        const toast = new bootstrap.Toast(toastContainer, { delay: 3000 });
        toast.show();
        toastContainer.addEventListener('hidden.bs.toast', () => toastContainer.remove());
    }
    function appendLog(message, level) {
        if (logConsole) {
            const timestamp = new Date().toLocaleTimeString();
            logConsole.innerHTML += `<span class="text-${level}">${timestamp} [${level.toUpperCase()}] ${message}</span>\n`;
            logConsole.scrollTop = logConsole.scrollHeight; // 滚动到底部
        }
    }

    // --- 辅助函数：显示/隐藏静态IP字段 ---
    function toggleStaticIpFields(show) {
        const staticIpFields = document.getElementById('static-ip-fields');
        if (staticIpFields) {
            staticIpFields.style.display = show ? 'block' : 'none';
        }
    }

    // --- 事件监听器绑定 ---
    // 密码显示/隐藏切换
    if (togglePasswordVisibilityBtn) {
        togglePasswordVisibilityBtn.addEventListener('click', () => {
            const type = wifiPasswordInput.getAttribute('type') === 'password' ? 'text' : 'password';
            wifiPasswordInput.setAttribute('type', type);
            passwordToggleIcon.classList.toggle('bi-eye');
            passwordToggleIcon.classList.toggle('bi-eye-slash');
        });
    }

    // --- 模态框确认逻辑 ---
    if (confirmationModalEl) {
        confirmationModalEl.addEventListener('show.bs.modal', (event) => {
            const button = event.relatedTarget;
            currentAction = button.getAttribute('data-action');
            
            const confirmBtn = document.getElementById('confirm-action-btn');
            confirmBtn.className = 'btn'; // Reset classes

            if (currentAction === 'reboot') {
                confirmationModalBody.textContent = '您确定要立即重启设备吗？';
                confirmBtn.classList.add('btn-danger');
            } else if (currentAction === 'factory_reset') {
                confirmationModalBody.textContent = '警告：您确定要恢复出厂设置吗？所有已保存的配置都将被擦除！';
                confirmBtn.classList.add('btn-warning');
            }
             confirmBtn.textContent = '确认';
        });
    }

    if (confirmActionBtn) {
        confirmActionBtn.addEventListener('click', () => {
            if (currentAction) {
                sendCommand({ command: currentAction });
                currentAction = null; // Reset action
            }
            confirmationModal.hide();
        });
    }

    // 初始加载时，根据默认选中的WiFi模式显示/隐藏字段
    if (wifiModeSelect) {
        wifiModeSelect.dispatchEvent(new Event('change'));
    }

    // 初始加载时，根据静态IP开关状态显示/隐藏字段
    const initialStaticIpToggle = document.getElementById('static-ip-toggle');
    if (initialStaticIpToggle) {
        toggleStaticIpFields(initialStaticIpToggle.checked);
    }

    if (scanWifiBtn) {
        scanWifiBtn.addEventListener('click', () => sendCommand({ command: 'scan_wifi_networks' }));
    }

    if (wifiForm) {
        // 监听WiFi模式切换
        if (wifiModeSelect) {
            wifiModeSelect.addEventListener('change', (e) => {
                const selectedMode = parseInt(e.target.value);
                const staFields = document.getElementById('sta-fields');
                const apFields = document.getElementById('ap-fields');
                const staticIpContainer = document.getElementById('static-ip-container');

                // 根据模式显示或隐藏相关字段
                if (selectedMode === 1) { // STA only
                    if (staFields) staFields.style.display = 'block';
                    if (apFields) apFields.style.display = 'none';
                    if (staticIpContainer) staticIpContainer.style.display = 'block';
                } else if (selectedMode === 2) { // AP only
                    if (staFields) staFields.style.display = 'none';
                    if (apFields) apFields.style.display = 'block';
                    if (staticIpContainer) staticIpContainer.style.display = 'none';
                } else if (selectedMode === 3) { // AP+STA
                    if (staFields) staFields.style.display = 'block';
                    if (apFields) apFields.style.display = 'block';
                    if (staticIpContainer) staticIpContainer.style.display = 'block';
                }
            });
        }

        // 监听静态IP开关
        const staticIpToggle = document.getElementById('static-ip-toggle');
        if (staticIpToggle) {
            staticIpToggle.addEventListener('change', (e) => {
                toggleStaticIpFields(e.target.checked);
            });
        }

        wifiForm.addEventListener('submit', (e) => {
            e.preventDefault();
            const settings = {
                ssid: document.getElementById('wifi-ssid').value,
                password: document.getElementById('wifi-password').value,
                ap_ssid: document.getElementById('ap-ssid').value,
                ap_password: document.getElementById('ap-password').value,
                mode: parseInt(wifiModeSelect.value),
                static_ip: document.getElementById('static-ip-toggle').checked,
                ip: document.getElementById('static-ip-address').value,
                subnet: document.getElementById('static-ip-subnet').value,
                gateway: document.getElementById('static-ip-gateway').value
            };
            sendCommand({ command: 'save_wifi_settings', data: settings });
        });
    }

    // --- 初始化 ---
    initWebSocket();
});
