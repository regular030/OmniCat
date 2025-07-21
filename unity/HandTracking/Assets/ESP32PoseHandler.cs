using System;
using System.Net.Sockets;
using System.Text;
using UnityEngine;
using TMPro;
using System.Collections;

public class ESP32PoseHandler : MonoBehaviour
{
    private int switchModeIndex = 0;
    private readonly string[] switchModes = { "Mode 1", "Mode 2", "Mode 3" };

    public string esp32IP = "192.168.1.100";
    public int port = 4210;
    public TextMeshProUGUI statusText;

    private UdpClient client;
    private Coroutine sendLoop;
    private string currentCommand = "";
    private string currentGesture = "";
    private bool isStopped = true;

    private enum InputMode { Hand, Controller }
    private InputMode currentMode = InputMode.Hand;

    void Start()
    {
        client = new UdpClient();
        UpdateStatus("🟡 Mode: Hand (default)");
    }

    void Update()
    {
        if (Input.GetKeyDown(KeyCode.JoystickButton3))
            ToggleInputMode();

        if (currentMode == InputMode.Controller)
            HandleControllerInput();
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

        if (Mathf.Abs(vertical) < 0.2f) vertical = 0;
        if (Mathf.Abs(horizontal) < 0.2f) horizontal = 0;

        if (vertical > 0.5f)
            StartContinuousCommand("Controller_Up", "back");
        else if (vertical < -0.5f)
            StartContinuousCommand("Controller_Down", "forward");
        else if (horizontal < -0.5f)
            StartContinuousCommand("Controller_Left", "left");
        else if (horizontal > 0.5f)
            StartContinuousCommand("Controller_Right", "right");
        else
            StopCommand("Controller_Stop", "stop");

        if (Input.GetKeyDown(KeyCode.JoystickButton0))
            SendSingleCommand("Controller_X", "shoot");
        if (Input.GetKeyDown(KeyCode.JoystickButton1))
            SendSingleCommand("Controller_O", "nod");
    }

    private void StartContinuousCommand(string gesture, string command)
    {
        if (!isStopped && currentCommand == command) return;

        StopExistingLoop();

        currentGesture = gesture;
        currentCommand = command;
        sendLoop = StartCoroutine(SendRepeatedly());
        isStopped = false;
    }

    private void StopCommand(string gesture, string command)
    {
        if (isStopped) return;

        StopExistingLoop();

        SendSingleCommand(gesture, command);
        currentCommand = "";
        isStopped = true;
    }

    private void StopExistingLoop()
    {
        if (sendLoop != null)
        {
            StopCoroutine(sendLoop);
            sendLoop = null;
        }
    }

    private void SendSingleCommand(string gesture, string command)
    {
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
    }

    private IEnumerator SendRepeatedly()
    {
        while (true)
        {
            SendSingleCommand(currentGesture, currentCommand);
            yield return new WaitForSeconds(0.5f);
        }
    }

    private void UpdateStatus(string message)
    {
        if (statusText != null)
            statusText.text = message;
    }

    // === Called from Unity Events (for hand tracking) ===
    public void HandlePalm() { if (currentMode == InputMode.Hand) StartContinuousCommand("Palm", "forward"); }
    public void HandleFist() { if (currentMode == InputMode.Hand) StartContinuousCommand("Fist", "back"); }
    public void HandlePinky()
    {
        if (currentMode != InputMode.Hand) return;

        // Send "switch" command only once
        SendSingleCommand("HangTen", "switch");

        // Show current switch mode in status
        string currentModeText = $"🔄 Switched to {switchModes[switchModeIndex]}";
        Debug.Log(currentModeText);
        UpdateStatus(currentModeText);

        // Advance the mode index (loop back to 0 after 3)
        switchModeIndex = (switchModeIndex + 1) % switchModes.Length;
    }
    public void HandlePeace() { if (currentMode == InputMode.Hand) StartContinuousCommand("Peace", "left"); }
    public void HandlePointer() { if (currentMode == InputMode.Hand) StartContinuousCommand("Pointer", "right"); }
    public void HandleNone() { if (currentMode == InputMode.Hand) StopCommand("None", "stop"); }

    void OnApplicationQuit()
    {
        client.Close();
    }
}
