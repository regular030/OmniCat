using System;
using System.Net.Sockets;
using System.Text;
using UnityEngine;
using TMPro;
using System.Collections;

public class ESP32PoseHandler : MonoBehaviour
{
    public string esp32IP = "192.168.1.100";
    public int port = 4210;
    public TextMeshProUGUI statusText;

    private UdpClient client;
    private string lastCommand = "";
    private bool isSending = false;

    private enum InputMode { Hand, Controller }
    private InputMode currentMode = InputMode.Hand;

    void Start()
    {
        client = new UdpClient();
        UpdateStatus("🟡 Mode: Hand (default)");
    }

    void Update()
    {
        // Triangle = JoystickButton3
        if (Input.GetKeyDown(KeyCode.JoystickButton3))
        {
            ToggleInputMode();
        }

        if (currentMode == InputMode.Controller)
        {
            HandleControllerInput();
        }
    }

    private void ToggleInputMode()
    {
        currentMode = currentMode == InputMode.Hand ? InputMode.Controller : InputMode.Hand;
        string modeText = currentMode == InputMode.Hand ? "🟡 Mode: Hand" : "🟣 Mode: Controller";
        Debug.Log($"🔁 Switched to {currentMode} mode");
        UpdateStatus(modeText);
    }

    private void HandleControllerInput()
    {
        float vertical = Input.GetAxis("Vertical");
        float horizontal = Input.GetAxis("Horizontal");

        // Round small movements to 0 to avoid accidental drift
        if (Mathf.Abs(vertical) < 0.2f) vertical = 0;
        if (Mathf.Abs(horizontal) < 0.2f) horizontal = 0;

        // Prioritize vertical before horizontal
        if (vertical > 0.5f)
            SendCommand("Controller_Up", "back");
        else if (vertical < -0.5f)
            SendCommand("Controller_Down", "forwards");
        else if (horizontal < -0.5f)
            SendCommand("Controller_Left", "left");
        else if (horizontal > 0.5f)
            SendCommand("Controller_Right", "right");
        else
            SendCommand("Controller_Stop", "stop");

        // Extra: X button to shoot
        if (Input.GetKeyDown(KeyCode.JoystickButton0))
            SendCommand("Controller_X", "shoot");
    }


    private void SendCommand(string gesture, string command)
    {
        if (command == lastCommand || isSending) return;
        StartCoroutine(SendWithDelay(gesture, command));
    }

    private IEnumerator SendWithDelay(string gesture, string command)
    {
        isSending = true;
        yield return new WaitForSeconds(0.01f); // 10ms delay
        lastCommand = command;

        try
        {
            byte[] data = Encoding.UTF8.GetBytes(command);
            client.Send(data, data.Length, esp32IP, port);
            string message = $"✅ Sent \"{gesture}\" → \"{command}\"";
            Debug.Log(message);
            UpdateStatus(message);
        }
        catch (Exception e)
        {
            string error = $"❌ Failed to send \"{gesture}\" → \"{command}\": {e.Message}";
            Debug.LogError(error);
            UpdateStatus(error);
        }

        isSending = false;
    }

    private void UpdateStatus(string message)
    {
        if (statusText != null)
        {
            statusText.text = message;
        }
    }

    // === Called from Unity Events (for hand tracking) ===
    public void HandlePalm() { if (currentMode == InputMode.Hand) SendCommand("Palm", "forward"); }
    public void HandleFist() { if (currentMode == InputMode.Hand) SendCommand("Fist", "back"); }
    public void HandlePinky() { if (currentMode == InputMode.Hand) SendCommand("HangTen", "switch"); }
    public void HandlePeace() { if (currentMode == InputMode.Hand) SendCommand("Peace", "left"); }
    public void HandlePointer() { if (currentMode == InputMode.Hand) SendCommand("Pointer", "right"); }
    public void HandleNone() { if (currentMode == InputMode.Hand) SendCommand("None", "stop"); }

    void OnApplicationQuit()
    {
        client.Close();
    }
}
