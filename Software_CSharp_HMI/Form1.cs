using System;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.Drawing;
using System.IO;
using System.IO.Ports;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using System.Windows.Forms.DataVisualization.Charting;

namespace STM32_PID_Tuner
{
    public partial class Form1 : Form
    {
        #region --- UI Components ---
        private Panel panelLeft, panelRight, panelTop, panelAnalyzer;
        private GroupBox grpCom, grpPid, grpAnalysis, grpChartControl, grpData;
        private ComboBox cbPorts, cbBaudrates;
        private Button btnConnect, btnDisconnect, btnUpdate, btnClear, btnLog, btnSaveProfile, btnLoadProfile;
        private NumericUpDown nudKp, nudKi, nudKd, nudSp;
        private CheckBox chkShowRpm, chkShowSp, chkShowPwm, chkShowErr;
        private Label lblOvershoot, lblSettlingTime, lblRiseTime, lblPeakRpm, lblSteadyErr, lblCurrentErr;
        private TextBox txtMonitor;
        private Chart chartPID;
        #endregion

        #region --- Core Variables ---
        private SerialPort serialPort;
        private ConcurrentQueue<string> rxQueue = new ConcurrentQueue<string>(); // Buffer 
        private Timer uiUpdateTimer; 

        private int timeTick = 0;
        private const int MAX_CHART_POINTS = 200;

        // Data Logging
        private bool isLogging = false;
        private StringBuilder logData = new StringBuilder();

        // Analyzer
        private PidAnalyzer analyzer = new PidAnalyzer();
        #endregion

        public Form1()
        {
            InitializeCustomComponent();
            InitializeSystem();
        }

        #region --- Initialization ---
        private void InitializeSystem()
        {
            serialPort = new SerialPort();
            serialPort.DataReceived += SerialPort_DataReceived;
            serialPort.ErrorReceived += (s, e) => { /* Xử lý nhiễu/lỗi COM */ };

            RefreshComPorts();
            cbBaudrates.Items.AddRange(new string[] { "9600", "19200", "38400", "57600", "115200", "256000", "500000" });
            cbBaudrates.SelectedItem = "115200";

            
            uiUpdateTimer = new Timer { Interval = 33 };
            uiUpdateTimer.Tick += UiUpdateTimer_Tick;
        }

        private void RefreshComPorts()
        {
            cbPorts.Items.Clear();
            cbPorts.Items.AddRange(SerialPort.GetPortNames());
            if (cbPorts.Items.Count > 0) cbPorts.SelectedIndex = 0;
        }
        #endregion

        #region --- Serial & Queue Logic ---
        private void BtnConnect_Click(object sender, EventArgs e)
        {
            try
            {
                if (cbPorts.SelectedItem == null) return;
                serialPort.PortName = cbPorts.SelectedItem.ToString();
                serialPort.BaudRate = int.Parse(cbBaudrates.SelectedItem.ToString());
                serialPort.NewLine = "\n";
                serialPort.Open();

                btnConnect.Enabled = false;
                btnDisconnect.Enabled = true;
                uiUpdateTimer.Start();
                txtMonitor.AppendText($">>> CONNECTED TO {serialPort.PortName} @ {serialPort.BaudRate}\r\n");
            }
            catch (Exception ex)
            {
                MessageBox.Show("Lỗi kết nối COM: " + ex.Message, "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
        }

        private void BtnDisconnect_Click(object sender, EventArgs e)
        {
            if (serialPort.IsOpen)
            {
                uiUpdateTimer.Stop();
                serialPort.Close();
                btnConnect.Enabled = true;
                btnDisconnect.Enabled = false;
                txtMonitor.AppendText(">>> DISCONNECTED\r\n");
            }
        }

       
        private void SerialPort_DataReceived(object sender, SerialDataReceivedEventArgs e)
        {
            try
            {
                while (serialPort.BytesToRead > 0)
                {
                    string rawData = serialPort.ReadLine().Trim();
                    if (!string.IsNullOrEmpty(rawData))
                    {
                        rxQueue.Enqueue(rawData); 
                    }
                }
            }
            catch { /* Bỏ qua lỗi rác khi cáp lỏng */ }
        }
        #endregion

        #region --- UI Update & Processing (Timer) ---
        private void UiUpdateTimer_Tick(object sender, EventArgs e)
        {
            if (rxQueue.IsEmpty) return;

            StringBuilder sbMonitor = new StringBuilder();
            bool chartNeedsUpdate = false;

            chartPID.Series["Setpoint"].Points.SuspendUpdates();
            chartPID.Series["Actual RPM"].Points.SuspendUpdates();
            chartPID.Series["PWM"].Points.SuspendUpdates();
            chartPID.Series["Error"].Points.SuspendUpdates();

            while (rxQueue.TryDequeue(out string data))
            {
                sbMonitor.AppendLine(data);

                if (isLogging) logData.AppendLine($"{DateTime.Now:HH:mm:ss.fff},{data}");

                // Parse: setpoint,rpm,pwm
                string[] parts = data.Split(',');
                if (parts.Length >= 3 &&
                    double.TryParse(parts[0], out double setpoint) &&
                    double.TryParse(parts[1], out double rpm) &&
                    double.TryParse(parts[2], out double pwm))
                {
                    double error = setpoint - rpm;

                 
                    analyzer.ProcessDataPoint(setpoint, rpm, timeTick * 0.01); // update analyzer (time in seconds)

                    //  chart
                    chartPID.Series["Setpoint"].Points.AddXY(timeTick, setpoint);
                    chartPID.Series["Actual RPM"].Points.AddXY(timeTick, rpm);
                    chartPID.Series["PWM"].Points.AddXY(timeTick, pwm);
                    chartPID.Series["Error"].Points.AddXY(timeTick, error);
                    timeTick++;
                    chartNeedsUpdate = true;
                }
            }

            if (chartNeedsUpdate)
            {
                CleanChartPoints();
                UpdateAnalysisUI();

                chartPID.Series["Setpoint"].Points.ResumeUpdates();
                chartPID.Series["Actual RPM"].Points.ResumeUpdates();
                chartPID.Series["PWM"].Points.ResumeUpdates();
                chartPID.Series["Error"].Points.ResumeUpdates();
            }

            
            if (sbMonitor.Length > 0)
            {
                if (txtMonitor.TextLength > 10000) txtMonitor.Clear(); 
                txtMonitor.AppendText(sbMonitor.ToString());
            }
        }

        private void CleanChartPoints()
        {
            // Scrolling logic
            if (chartPID.Series["Setpoint"].Points.Count > MAX_CHART_POINTS)
            {
                chartPID.Series["Setpoint"].Points.RemoveAt(0);
                chartPID.Series["Actual RPM"].Points.RemoveAt(0);
                chartPID.Series["PWM"].Points.RemoveAt(0);
                chartPID.Series["Error"].Points.RemoveAt(0);

                var axisX = chartPID.ChartAreas[0].AxisX;
                axisX.Minimum = chartPID.Series["Setpoint"].Points[0].XValue;
                axisX.Maximum = chartPID.Series["Setpoint"].Points.Last().XValue;
            }
        }

        private void UpdateAnalysisUI()
        {
            lblOvershoot.Text = $"Overshoot: {analyzer.Overshoot:F2} %";
            lblSettlingTime.Text = $"Settling Time (Ts): {(analyzer.SettlingTime > 0 ? analyzer.SettlingTime.ToString("F3") + " s" : "---")}";
            lblRiseTime.Text = $"Rise Time (Tr): {(analyzer.RiseTime > 0 ? analyzer.RiseTime.ToString("F3") + " s" : "---")}";
            lblPeakRpm.Text = $"Peak RPM: {analyzer.PeakRpm:F1}";
            lblSteadyErr.Text = $"Steady Err: {analyzer.SteadyStateError:F2}";
            lblCurrentErr.Text = $"Current Err: {analyzer.CurrentError:F2}";
        }
        #endregion

        #region --- Controls & Features ---
        private void BtnUpdate_Click(object sender, EventArgs e)
        {
            if (serialPort.IsOpen)
            {
                try
                {
                    serialPort.Write($"KP={nudKp.Value}\nKI={nudKi.Value}\nKD={nudKd.Value}\nSP={nudSp.Value}\n");
                    analyzer.Reset((double)nudSp.Value); // Reset analyzer khi đổi setpoint
                    txtMonitor.AppendText($">>> SENT PID: KP={nudKp.Value}, KI={nudKi.Value}, KD={nudKd.Value}, SP={nudSp.Value}\r\n");
                }
                catch (Exception ex) { MessageBox.Show("Lỗi gửi lệnh: " + ex.Message); }
            }
            else { MessageBox.Show("Chưa kết nối COM!"); }
        }

        private void BtnClear_Click(object sender, EventArgs e)
        {
            foreach (var series in chartPID.Series) series.Points.Clear();
            timeTick = 0;
            analyzer.Reset((double)nudSp.Value);
            chartPID.ChartAreas[0].AxisX.Minimum = double.NaN;
            chartPID.ChartAreas[0].AxisX.Maximum = double.NaN;
        }

        private void BtnLog_Click(object sender, EventArgs e)
        {
            isLogging = !isLogging;
            if (isLogging)
            {
                btnLog.Text = "Stop Logging";
                btnLog.BackColor = Color.Orange;
                logData.Clear();
                logData.AppendLine("Time,Setpoint,RPM,PWM");
            }
            else
            {
                btnLog.Text = "Start Logging";
                btnLog.BackColor = Color.LightGray;
                SaveFileDialog sfd = new SaveFileDialog { Filter = "CSV file (*.csv)|*.csv", FileName = "PID_Log.csv" };
                if (sfd.ShowDialog() == DialogResult.OK)
                {
                    File.WriteAllText(sfd.FileName, logData.ToString());
                    MessageBox.Show("Đã lưu Log thành công!");
                }
            }
        }

        private void BtnSaveProfile_Click(object sender, EventArgs e)
        {
            SaveFileDialog sfd = new SaveFileDialog { Filter = "PID Profile (*.txt)|*.txt" };
            if (sfd.ShowDialog() == DialogResult.OK)
            {
                File.WriteAllText(sfd.FileName, $"{nudKp.Value};{nudKi.Value};{nudKd.Value};{nudSp.Value}");
            }
        }

        private void BtnLoadProfile_Click(object sender, EventArgs e)
        {
            OpenFileDialog ofd = new OpenFileDialog { Filter = "PID Profile (*.txt)|*.txt" };
            if (ofd.ShowDialog() == DialogResult.OK)
            {
                try
                {
                    string[] parts = File.ReadAllText(ofd.FileName).Split(';');
                    nudKp.Value = decimal.Parse(parts[0]);
                    nudKi.Value = decimal.Parse(parts[1]);
                    nudKd.Value = decimal.Parse(parts[2]);
                    nudSp.Value = decimal.Parse(parts[3]);
                }
                catch { MessageBox.Show("File profile không hợp lệ!"); }
            }
        }

        private void ChkSeries_CheckedChanged(object sender, EventArgs e)
        {
            chartPID.Series["Setpoint"].Enabled = chkShowSp.Checked;
            chartPID.Series["Actual RPM"].Enabled = chkShowRpm.Checked;
            chartPID.Series["PWM"].Enabled = chkShowPwm.Checked;
            chartPID.Series["Error"].Enabled = chkShowErr.Checked;

            // Auto scale Y axis
            chartPID.ChartAreas[0].RecalculateAxesScale();
        }
        #endregion

        #region --- UI Code-Behind Generation ---
        private void InitializeCustomComponent()
        {
            this.Text = "STM32 Real-time PID Tuning & Analysis";
            this.Size = new Size(1300, 750);
            this.Font = new Font("Segoe UI", 9F, FontStyle.Regular, GraphicsUnit.Point);
            this.StartPosition = FormStartPosition.CenterScreen;
            this.BackColor = Color.FromArgb(240, 240, 240);

            // ANEL LEFT 
            panelLeft = new Panel() { Dock = DockStyle.Left, Width = 280, Padding = new Padding(10) };
            this.Controls.Add(panelLeft);

            //  COM SETUP
            grpCom = new GroupBox() { Text = "1. Connection", Dock = DockStyle.Top, Height = 90 };
            cbPorts = new ComboBox() { Location = new Point(10, 25), Width = 110, DropDownStyle = ComboBoxStyle.DropDownList };
            cbBaudrates = new ComboBox() { Location = new Point(130, 25), Width = 110, DropDownStyle = ComboBoxStyle.DropDownList };
            btnConnect = new Button() { Text = "Connect", Location = new Point(10, 55), Width = 110, BackColor = Color.LightGreen };
            btnDisconnect = new Button() { Text = "Disconnect", Location = new Point(130, 55), Width = 110, BackColor = Color.LightCoral, Enabled = false };
            btnConnect.Click += BtnConnect_Click;
            btnDisconnect.Click += BtnDisconnect_Click;
            grpCom.Controls.AddRange(new Control[] { cbPorts, cbBaudrates, btnConnect, btnDisconnect });
            panelLeft.Controls.Add(grpCom);

            // PID SETUP
            grpPid = new GroupBox() { Text = "2. PID Parameters", Dock = DockStyle.Top, Height = 210 };
            panelLeft.Controls.Add(grpPid);
            grpPid.BringToFront();

            int y = 25; int gap = 30;
            grpPid.Controls.Add(new Label() { Text = "Setpoint:", Location = new Point(10, y), Width = 80 });
            nudSp = new NumericUpDown() { Location = new Point(90, y - 2), Width = 150, Maximum = 10000, Minimum = -10000, DecimalPlaces = 1 };
            grpPid.Controls.Add(new Label() { Text = "Kp:", Location = new Point(10, y += gap), Width = 80 });
            nudKp = new NumericUpDown() { Location = new Point(90, y - 2), Width = 150, Maximum = 10000, DecimalPlaces = 4, Increment = 0.05M };
            grpPid.Controls.Add(new Label() { Text = "Ki:", Location = new Point(10, y += gap), Width = 80 });
            nudKi = new NumericUpDown() { Location = new Point(90, y - 2), Width = 150, Maximum = 10000, DecimalPlaces = 4, Increment = 0.01M };
            grpPid.Controls.Add(new Label() { Text = "Kd:", Location = new Point(10, y += gap), Width = 80 });
            nudKd = new NumericUpDown() { Location = new Point(90, y - 2), Width = 150, Maximum = 10000, DecimalPlaces = 4, Increment = 0.05M };

            btnUpdate = new Button() { Text = "🚀 UPDATE PID TO STM32", Location = new Point(10, y += gap), Width = 230, Height = 35, BackColor = Color.DodgerBlue, ForeColor = Color.White, Font = new Font(this.Font, FontStyle.Bold) };
            btnUpdate.Click += BtnUpdate_Click;

            btnSaveProfile = new Button() { Text = "Save", Location = new Point(10, y += 40), Width = 110 };
            btnLoadProfile = new Button() { Text = "Load", Location = new Point(130, y), Width = 110 };
            btnSaveProfile.Click += BtnSaveProfile_Click;
            btnLoadProfile.Click += BtnLoadProfile_Click;

            grpPid.Controls.AddRange(new Control[] { nudSp, nudKp, nudKi, nudKd, btnUpdate, btnSaveProfile, btnLoadProfile });

            // ANALYSIS METRICS
            grpAnalysis = new GroupBox() { Text = "3. Realtime Analysis (±5% Band)", Dock = DockStyle.Top, Height = 170 };
            panelLeft.Controls.Add(grpAnalysis);
            grpAnalysis.BringToFront();

            lblCurrentErr = new Label() { Text = "Current Err: ---", Location = new Point(10, 25), Width = 230, ForeColor = Color.DarkRed, Font = new Font(this.Font, FontStyle.Bold) };
            lblSteadyErr = new Label() { Text = "Steady Err: ---", Location = new Point(10, 45), Width = 230 };
            lblOvershoot = new Label() { Text = "Overshoot: ---", Location = new Point(10, 65), Width = 230 };
            lblPeakRpm = new Label() { Text = "Peak RPM: ---", Location = new Point(10, 85), Width = 230 };
            lblRiseTime = new Label() { Text = "Rise Time (Tr): ---", Location = new Point(10, 105), Width = 230 };
            lblSettlingTime = new Label() { Text = "Settling Time (Ts): ---", Location = new Point(10, 125), Width = 230, ForeColor = Color.DarkGreen, Font = new Font(this.Font, FontStyle.Bold) };

            grpAnalysis.Controls.AddRange(new Control[] { lblCurrentErr, lblSteadyErr, lblOvershoot, lblPeakRpm, lblRiseTime, lblSettlingTime });

            // PANEL RIGHT (Chart & Terminal)
            panelRight = new Panel() { Dock = DockStyle.Fill, Padding = new Padding(10) };
            this.Controls.Add(panelRight);

            // Toolbar 
            panelTop = new Panel() { Dock = DockStyle.Top, Height = 40 };
            panelRight.Controls.Add(panelTop);

            chkShowSp = new CheckBox() { Text = "Setpoint", Checked = true, Location = new Point(10, 10), Width = 80, ForeColor = Color.Red };
            chkShowRpm = new CheckBox() { Text = "RPM", Checked = true, Location = new Point(90, 10), Width = 60, ForeColor = Color.Blue };
            chkShowPwm = new CheckBox() { Text = "PWM", Checked = false, Location = new Point(150, 10), Width = 60, ForeColor = Color.Orange };
            chkShowErr = new CheckBox() { Text = "Error", Checked = false, Location = new Point(210, 10), Width = 60, ForeColor = Color.Purple };

            chkShowSp.CheckedChanged += ChkSeries_CheckedChanged;
            chkShowRpm.CheckedChanged += ChkSeries_CheckedChanged;
            chkShowPwm.CheckedChanged += ChkSeries_CheckedChanged;
            chkShowErr.CheckedChanged += ChkSeries_CheckedChanged;

            btnClear = new Button() { Text = "Clear Graph", Location = new Point(300, 5), Width = 100 };
            btnLog = new Button() { Text = "Start Logging", Location = new Point(410, 5), Width = 100 };
            btnClear.Click += BtnClear_Click;
            btnLog.Click += BtnLog_Click;

            panelTop.Controls.AddRange(new Control[] { chkShowSp, chkShowRpm, chkShowPwm, chkShowErr, btnClear, btnLog });

            // Chart
            chartPID = new Chart() { Dock = DockStyle.Fill };
            ChartArea ca = new ChartArea("MainArea");
            ca.AxisX.Title = "Time";
            ca.AxisY.Title = "Value";
            ca.AxisX.MajorGrid.LineColor = Color.LightGray;
            ca.AxisY.MajorGrid.LineColor = Color.LightGray;

            //  Zoom & Pan
            ca.CursorX.IsUserEnabled = true;
            ca.CursorX.IsUserSelectionEnabled = true;
            ca.AxisX.ScaleView.Zoomable = true;
            ca.AxisX.ScrollBar.IsPositionedInside = true;

            chartPID.ChartAreas.Add(ca);

            // line
            chartPID.Series.Add(new Series("Setpoint") { ChartType = SeriesChartType.Line, Color = Color.Red, BorderWidth = 2 });
            chartPID.Series.Add(new Series("Actual RPM") { ChartType = SeriesChartType.Line, Color = Color.Blue, BorderWidth = 2 });
            chartPID.Series.Add(new Series("PWM") { ChartType = SeriesChartType.FastLine, Color = Color.Orange, BorderWidth = 1, Enabled = false }); // PWM nhảy nhanh, dùng FastLine
            chartPID.Series.Add(new Series("Error") { ChartType = SeriesChartType.FastLine, Color = Color.Purple, BorderWidth = 1, Enabled = false, BorderDashStyle = ChartDashStyle.Dash });

            chartPID.Legends.Add(new Legend() { Docking = Docking.Top, Alignment = StringAlignment.Center });
            panelRight.Controls.Add(chartPID);
            chartPID.BringToFront();

            // Monitor
            txtMonitor = new TextBox()
            {
                Dock = DockStyle.Bottom,
                Height = 120,
                Multiline = true,
                ScrollBars = ScrollBars.Vertical,
                BackColor = Color.FromArgb(30, 30, 30),
                ForeColor = Color.Lime,
                Font = new Font("Consolas", 10)
            };
            panelRight.Controls.Add(txtMonitor);
        }

        protected override void OnFormClosing(FormClosingEventArgs e)
        {
            if (serialPort != null && serialPort.IsOpen) serialPort.Close();
            if (uiUpdateTimer != null) uiUpdateTimer.Stop();
            base.OnFormClosing(e);
        }
        #endregion
    }

    #region --- PID Response Analyzer Logic ---
    
    public class PidAnalyzer
    {
        public double Overshoot { get; private set; }
        public double SettlingTime { get; private set; }
        public double RiseTime { get; private set; }
        public double PeakRpm { get; private set; }
        public double SteadyStateError { get; private set; }
        public double CurrentError { get; private set; }

        private double _targetSetpoint;
        private double _currentTime;
        private double _stepStartTime;

       
        private bool _isRising = false;
        private double _t10 = -1;
        private double _t90 = -1;
        private double _lastTimeOutsideBand = 0;

        
        private bool _isSettled = false;

        public PidAnalyzer() { Reset(0, 0); }

        public void Reset(double newSetpoint)
        {
            Reset(newSetpoint, _currentTime);
        }

        public void Reset(double newSetpoint, double currentTime)
        {
            _targetSetpoint = Math.Abs(newSetpoint);
            _stepStartTime = currentTime;

            Overshoot = 0;
            SettlingTime = 0;
            RiseTime = 0;
            PeakRpm = 0;
            SteadyStateError = 0;

            _isRising = false;
            _t10 = -1;
            _t90 = -1;
            _lastTimeOutsideBand = currentTime;

           
            _isSettled = false;
        }

        public void ProcessDataPoint(double setpoint, double actualRpm, double time)
        {
            _currentTime = time;
            actualRpm = Math.Abs(actualRpm);
            double target = Math.Abs(setpoint);

            CurrentError = target - actualRpm;

            if (target <= 0.1) return;

            if (target != _targetSetpoint)
            {
                Reset(target, time);
            }

            //  Peak & Overshoot
            if (actualRpm > PeakRpm)
            {
                PeakRpm = actualRpm;
                if (PeakRpm > target)
                {
                    Overshoot = ((PeakRpm - target) / target) * 100.0;
                }
            }

            //  Rise Time (10% to 90%)
            if (_t10 < 0 && actualRpm >= target * 0.1) _t10 = time;
            if (_t90 < 0 && actualRpm >= target * 0.9) _t90 = time;

            if (_t10 > 0 && _t90 > 0 && RiseTime == 0)
            {
                RiseTime = _t90 - _t10;
            }

            //Settling Time 
            double lowerBand = target * 0.95;
            double upperBand = target * 1.05;

            if (actualRpm < lowerBand || actualRpm > upperBand)
            {
                _lastTimeOutsideBand = time;
                SteadyStateError = CurrentError;

               
                if (!_isSettled)
                {
                    SettlingTime = 0;
                }
            }
            else
            {
                
                if (!_isSettled && (time - _lastTimeOutsideBand > 0.2))
                {
                    SettlingTime = _lastTimeOutsideBand - _stepStartTime;

                    
                    _isSettled = true;
                }

                SteadyStateError = (SteadyStateError * 0.9) + (CurrentError * 0.1);
            }
        }
    }
    #endregion
}
