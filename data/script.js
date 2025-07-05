document.addEventListener('DOMContentLoaded', () => {
    // 全局WebSocket实例
    let websocket;

    // --- DOM 元素选择 ---
    const navLinks = document.querySelectorAll('.nav-link[data-page]');
    const pages = document.querySelectorAll('.page-content');
    
    // 仪表盘页面元素
    const statusButton = document.getElementById('statusButton');
    const responseParagraph = document.getElementById('response');
    const statusContainer = document.getElementById('statusContainer');

    // WiFi设置页面元素
    const wifiForm = document.getElementById('wifi-form');
    const scanWifiBtn = document.getElementById('scan-wifi-btn');
    const wifiScanResultsBody = document.getElementById('wifi-scan-results');
    const wifiSsidInput = document.getElementById('wifi-ssid');
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
            }

            // --- 新增：在移动端视图下，点击链接后自动收起菜单 ---
            const navbarToggler = document.querySelector('.navbar-toggler');
            if (navbarToggler && getComputedStyle(navbarToggler).display !== 'none') {
                navbarToggler.click();
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
        };

        websocket.onclose = () => {
            console.log('WebSocket connection closed.');
            showToast('WebSocket 已断开，5秒后尝试重连...', 'danger');
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
            default:
                console.warn('Unknown message type received:', message.type);
        }
    }

    // --- UI 更新函数 ---
    function updateSystemStatusUI(data) {
        const formatBytes = (bytes) => {
            if (bytes === undefined) return 'N/A';
            if (bytes > 1024 * 1024) return `${(bytes / (1024 * 1024)).toFixed(2)} MB`;
            return `${(bytes / 1024).toFixed(2)} KB`;
        };
        statusContainer.innerHTML = `
            <ul class="list-group mt-3">
                <li class="list-group-item d-flex justify-content-between align-items-center">运行时间 <span class="badge bg-primary rounded-pill">${data.uptime} 秒</span></li>
                <li class="list-group-item d-flex justify-content-between align-items-center">通用堆内存 (Heap) <span class="badge bg-info rounded-pill">${formatBytes(data.heap_free)} / ${formatBytes(data.heap_total)}</span></li>
                <li class="list-group-item d-flex justify-content-between align-items-center">PSRAM 内存池 <span class="badge bg-success rounded-pill">${formatBytes(data.psram_pool_free)} / ${formatBytes(data.psram_pool_total)}</span></li>
            </ul>`;
    }

    function updateWifiForm(data) {
        wifiSsidInput.value = data.ssid || '';
        document.querySelector(`input[name="wifi-mode"][value="${data.mode}"]`).checked = true;
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

    // --- 事件监听器绑定 ---
    if (statusButton) {
        statusButton.addEventListener('click', () => sendCommand({ command: 'get_system_status' }));
    }

    if (scanWifiBtn) {
        scanWifiBtn.addEventListener('click', () => sendCommand({ command: 'scan_wifi_networks' }));
    }

    if (wifiForm) {
        wifiForm.addEventListener('submit', (e) => {
            e.preventDefault();
            const settings = {
                ssid: document.getElementById('wifi-ssid').value,
                password: document.getElementById('wifi-password').value,
                mode: parseInt(document.querySelector('input[name="wifi-mode"]:checked').value)
            };
            sendCommand({ command: 'save_wifi_settings', data: settings });
        });
    }

    // --- 初始化 ---
    initWebSocket();
});
