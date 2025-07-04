document.addEventListener('DOMContentLoaded', () => {
    const statusButton = document.getElementById('testButton');
    const responseParagraph = document.getElementById('response');
    const statusContainer = document.createElement('div');
    if(responseParagraph) {
        responseParagraph.after(statusContainer);
    }

    let websocket;

    function initWebSocket() {
        const wsUrl = `ws://${window.location.hostname}/ws`;
        console.log(`Connecting to WebSocket at: ${wsUrl}`);
        websocket = new WebSocket(wsUrl);

        websocket.onopen = (event) => {
            console.log('WebSocket connection opened:', event);
            if(responseParagraph) {
                responseParagraph.textContent = 'WebSocket 已连接';
                responseParagraph.style.color = 'green';
            }
        };

        websocket.onmessage = (event) => {
            console.log('WebSocket message received:', event.data);
            try {
                const message = JSON.parse(event.data);
                if (message.type === 'system_status') {
                    updateSystemStatusUI(message.data);
                } else if (message.error) {
                    if(responseParagraph) responseParagraph.textContent = `收到错误: ${message.error}`;
                }
            } catch (e) {
                console.error('Error parsing JSON:', e);
                if(responseParagraph) responseParagraph.textContent = `收到非JSON消息: ${event.data}`;
            }
        };

        websocket.onclose = (event) => {
            console.log('WebSocket connection closed:', event);
            if(responseParagraph) {
                responseParagraph.textContent = 'WebSocket 已断开，5秒后尝试重连...';
                responseParagraph.style.color = 'red';
            }
            statusContainer.innerHTML = '';
            setTimeout(initWebSocket, 5000);
        };

        websocket.onerror = (event) => {
            console.error('WebSocket error observed:', event);
        };
    }

    function updateSystemStatusUI(data) {
        // 辅助函数，用于格式化字节为KB或MB
        const formatBytes = (bytes) => {
            if (bytes === undefined) return 'N/A';
            if (bytes > 1024 * 1024) {
                return (bytes / (1024 * 1024)).toFixed(2) + ' MB';
            }
            return (bytes / 1024).toFixed(2) + ' KB';
        };

        statusContainer.innerHTML = `
            <h4 class="mt-3">系统状态</h4>
            <ul class="list-group">
                <li class="list-group-item d-flex justify-content-between align-items-center">
                    运行时间
                    <span class="badge bg-primary rounded-pill">${data.uptime} 秒</span>
                </li>
                <li class="list-group-item d-flex justify-content-between align-items-center">
                    通用堆内存 (Heap)
                    <span class="badge bg-info rounded-pill">${formatBytes(data.heap_free)} / ${formatBytes(data.heap_total)}</span>
                </li>
                <li class="list-group-item d-flex justify-content-between align-items-center">
                    PSRAM 内存池
                    <span class="badge bg-success rounded-pill">${formatBytes(data.psram_pool_free)} / ${formatBytes(data.psram_pool_total)}</span>
                </li>
            </ul>
        `;
    }

    if(statusButton) {
        statusButton.addEventListener('click', () => {
            if (websocket && websocket.readyState === WebSocket.OPEN) {
                const command = { command: 'get_system_status' };
                websocket.send(JSON.stringify(command));
                console.log('Sent command:', command);
                if(responseParagraph) responseParagraph.textContent = '已发送状态请求...';
            } else {
                if(responseParagraph) responseParagraph.textContent = 'WebSocket 未连接，无法发送请求。';
                console.warn('WebSocket is not open. readyState:', websocket ? websocket.readyState : 'uninitialized');
            }
        });
    }

    // 初始化WebSocket连接
    initWebSocket();
});
