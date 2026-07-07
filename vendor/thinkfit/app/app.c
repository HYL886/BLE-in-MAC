
void app_init(int deepRetWakeUp)
{
    // deepRetWakeUp = 1, system wake from deep sleep.
    // deepRetWakeUp = 0, system cold boot.
    // Add init codes.
}
void app_deinit(void)
{
    // Called,When system before enter deep sleep.
    // Add deinit codes.
}

void app_irq_proc(void)
{
    // Add irq codes.
}

void app_proc(void)
{
    // User process
}
