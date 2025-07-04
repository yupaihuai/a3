document.addEventListener('DOMContentLoaded', () => {
    const testButton = document.getElementById('testButton');
    const responseParagraph = document.getElementById('response');

    testButton.addEventListener('click', () => {
        responseParagraph.textContent = '按钮被点击了！';
        // 可以在这里添加WebSocket或Fetch API请求
    });
});