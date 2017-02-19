<Global.Microsoft.VisualBasic.CompilerServices.DesignerGenerated()>
Partial Class Form1
    Inherits System.Windows.Forms.Form

    'Form overrides dispose to clean up the component list.
    <System.Diagnostics.DebuggerNonUserCode()>
    Protected Overrides Sub Dispose(ByVal disposing As Boolean)
        Try
            If disposing AndAlso components IsNot Nothing Then
                components.Dispose()
            End If
        Finally
            MyBase.Dispose(disposing)
        End Try
    End Sub

    'Required by the Windows Form Designer
    Private components As System.ComponentModel.IContainer

    'NOTE: The following procedure is required by the Windows Form Designer
    'It can be modified using the Windows Form Designer.  
    'Do not modify it using the code editor.
    <System.Diagnostics.DebuggerStepThrough()>
    Private Sub InitializeComponent()
        Me.components = New System.ComponentModel.Container()
        Dim resources As System.ComponentModel.ComponentResourceManager = New System.ComponentModel.ComponentResourceManager(GetType(Form1))
        Me.grbx_serial_settings = New System.Windows.Forms.GroupBox()
        Me.btn_701xReset = New System.Windows.Forms.Button()
        Me.chkbx_rts = New System.Windows.Forms.CheckBox()
        Me.btn_portSearch = New System.Windows.Forms.Button()
        Me.btn_serialConnect = New System.Windows.Forms.Button()
        Me.combobx_portNo = New System.Windows.Forms.ComboBox()
        Me.combobx_baudRate = New System.Windows.Forms.ComboBox()
        Me.label3 = New System.Windows.Forms.Label()
        Me.label2 = New System.Windows.Forms.Label()
        Me.tab_ctrl_flashmode = New System.Windows.Forms.TabControl()
        Me.tabPage1 = New System.Windows.Forms.TabPage()
        Me.Label5 = New System.Windows.Forms.Label()
        Me.lbl_filename = New System.Windows.Forms.Label()
        Me.grbx_flashOption = New System.Windows.Forms.GroupBox()
        Me.Label10 = New System.Windows.Forms.Label()
        Me.chk_bx_load_to_cram = New System.Windows.Forms.CheckBox()
        Me.btn_abort = New System.Windows.Forms.Button()
        Me.ProgressBar1 = New System.Windows.Forms.ProgressBar()
        Me.btn_autoFlash = New System.Windows.Forms.Button()
        Me.groupBox5 = New System.Windows.Forms.GroupBox()
        Me.radiobtn_customBank = New System.Windows.Forms.RadioButton()
        Me.radiobtn_allBank = New System.Windows.Forms.RadioButton()
        Me.chkbx_b7 = New System.Windows.Forms.CheckBox()
        Me.chkbx_b6 = New System.Windows.Forms.CheckBox()
        Me.chkbx_b5 = New System.Windows.Forms.CheckBox()
        Me.chkbx_b4 = New System.Windows.Forms.CheckBox()
        Me.chkbx_b3 = New System.Windows.Forms.CheckBox()
        Me.chkbx_b2 = New System.Windows.Forms.CheckBox()
        Me.chkbx_b1 = New System.Windows.Forms.CheckBox()
        Me.chkbx_b0 = New System.Windows.Forms.CheckBox()
        Me.grbx_console = New System.Windows.Forms.GroupBox()
        Me.txtbx_console = New System.Windows.Forms.TextBox()
        Me.grbx_hexfilepath = New System.Windows.Forms.GroupBox()
        Me.label1 = New System.Windows.Forms.Label()
        Me.txtbx_fileName = New System.Windows.Forms.TextBox()
        Me.txtbx_filePath = New System.Windows.Forms.TextBox()
        Me.btn_pathBrowse = New System.Windows.Forms.Button()
        Me.tab_config = New System.Windows.Forms.TabPage()
        Me.btn_device_type = New System.Windows.Forms.Button()
        Me.txtbx_device_type = New System.Windows.Forms.TextBox()
        Me.Label8 = New System.Windows.Forms.Label()
        Me.btn_cin_clear = New System.Windows.Forms.Button()
        Me.txtbx_cin_clear = New System.Windows.Forms.TextBox()
        Me.Label7 = New System.Windows.Forms.Label()
        Me.lbl_key_length = New System.Windows.Forms.Label()
        Me.btn_commit = New System.Windows.Forms.Button()
        Me.btn_send_key = New System.Windows.Forms.Button()
        Me.btn_send_mac = New System.Windows.Forms.Button()
        Me.txtbx_defKey = New System.Windows.Forms.TextBox()
        Me.txtbx_mac_addr = New System.Windows.Forms.TextBox()
        Me.Label6 = New System.Windows.Forms.Label()
        Me.Label4 = New System.Windows.Forms.Label()
        Me.TabPage2 = New System.Windows.Forms.TabPage()
        Me.btn_save_macro = New System.Windows.Forms.Button()
        Me.txtbx_macro9 = New System.Windows.Forms.TextBox()
        Me.Label16 = New System.Windows.Forms.Label()
        Me.txtbx_macro8 = New System.Windows.Forms.TextBox()
        Me.Label17 = New System.Windows.Forms.Label()
        Me.txtbx_macro7 = New System.Windows.Forms.TextBox()
        Me.Label18 = New System.Windows.Forms.Label()
        Me.txtbx_macro6 = New System.Windows.Forms.TextBox()
        Me.Label19 = New System.Windows.Forms.Label()
        Me.txtbx_macro5 = New System.Windows.Forms.TextBox()
        Me.Label20 = New System.Windows.Forms.Label()
        Me.txtbx_macro4 = New System.Windows.Forms.TextBox()
        Me.Label15 = New System.Windows.Forms.Label()
        Me.txtbx_macro3 = New System.Windows.Forms.TextBox()
        Me.Label14 = New System.Windows.Forms.Label()
        Me.txtbx_macro2 = New System.Windows.Forms.TextBox()
        Me.Label13 = New System.Windows.Forms.Label()
        Me.txtbx_macro1 = New System.Windows.Forms.TextBox()
        Me.Label12 = New System.Windows.Forms.Label()
        Me.txtbx_macro0 = New System.Windows.Forms.TextBox()
        Me.Label11 = New System.Windows.Forms.Label()
        Me.Label9 = New System.Windows.Forms.Label()
        Me.SerialPort1 = New System.IO.Ports.SerialPort(Me.components)
        Me.FolderBrowserDialog1 = New System.Windows.Forms.FolderBrowserDialog()
        Me.MenuStrip1 = New System.Windows.Forms.MenuStrip()
        Me.FileToolStripMenuItem = New System.Windows.Forms.ToolStripMenuItem()
        Me.toolStripSeparator = New System.Windows.Forms.ToolStripSeparator()
        Me.toolStripSeparator1 = New System.Windows.Forms.ToolStripSeparator()
        Me.toolStripSeparator2 = New System.Windows.Forms.ToolStripSeparator()
        Me.ExitToolStripMenuItem = New System.Windows.Forms.ToolStripMenuItem()
        Me.EditToolStripMenuItem = New System.Windows.Forms.ToolStripMenuItem()
        Me.toolStripSeparator3 = New System.Windows.Forms.ToolStripSeparator()
        Me.toolStripSeparator4 = New System.Windows.Forms.ToolStripSeparator()
        Me.ConsoleClearToolStripMenuItem = New System.Windows.Forms.ToolStripMenuItem()
        Me.ToolsToolStripMenuItem = New System.Windows.Forms.ToolStripMenuItem()
        Me.ExpandToolStripMenuItem_file_path = New System.Windows.Forms.ToolStripMenuItem()
        Me.CustomizeToolStripMenuItem = New System.Windows.Forms.ToolStripMenuItem()
        Me.OptionsToolStripMenuItem = New System.Windows.Forms.ToolStripMenuItem()
        Me.PortConnectToolStripMenuItem = New System.Windows.Forms.ToolStripMenuItem()
        Me.ToolStripSeparator5 = New System.Windows.Forms.ToolStripSeparator()
        Me.ToolStripSeparator6 = New System.Windows.Forms.ToolStripSeparator()
        Me.ShrinkToolStripMenuItem_autoflash = New System.Windows.Forms.ToolStripMenuItem()
        Me.AbortToolStripMenuItem_abort = New System.Windows.Forms.ToolStripMenuItem()
        Me.WindowToolStripMenuItem = New System.Windows.Forms.ToolStripMenuItem()
        Me.ShrinkToolStripMenuItem1 = New System.Windows.Forms.ToolStripMenuItem()
        Me.ExpandToolStripMenuItem1 = New System.Windows.Forms.ToolStripMenuItem()
        Me.HelpToolStripMenuItem = New System.Windows.Forms.ToolStripMenuItem()
        Me.ShortcutKeysReferenceToolStripMenuItem = New System.Windows.Forms.ToolStripMenuItem()
        Me.OpenFileDialog1 = New System.Windows.Forms.OpenFileDialog()
        Me.SaveFileDialog1 = New System.Windows.Forms.SaveFileDialog()
        Me.FileSystemWatcher1 = New System.IO.FileSystemWatcher()
        Me.chkbx_boot_backup = New System.Windows.Forms.CheckBox()
        Me.grbx_serial_settings.SuspendLayout()
        Me.tab_ctrl_flashmode.SuspendLayout()
        Me.tabPage1.SuspendLayout()
        Me.grbx_flashOption.SuspendLayout()
        Me.groupBox5.SuspendLayout()
        Me.grbx_console.SuspendLayout()
        Me.grbx_hexfilepath.SuspendLayout()
        Me.tab_config.SuspendLayout()
        Me.TabPage2.SuspendLayout()
        Me.MenuStrip1.SuspendLayout()
        CType(Me.FileSystemWatcher1, System.ComponentModel.ISupportInitialize).BeginInit()
        Me.SuspendLayout()
        '
        'grbx_serial_settings
        '
        Me.grbx_serial_settings.Controls.Add(Me.btn_701xReset)
        Me.grbx_serial_settings.Controls.Add(Me.chkbx_rts)
        Me.grbx_serial_settings.Controls.Add(Me.btn_portSearch)
        Me.grbx_serial_settings.Controls.Add(Me.btn_serialConnect)
        Me.grbx_serial_settings.Controls.Add(Me.combobx_portNo)
        Me.grbx_serial_settings.Controls.Add(Me.combobx_baudRate)
        Me.grbx_serial_settings.Controls.Add(Me.label3)
        Me.grbx_serial_settings.Controls.Add(Me.label2)
        Me.grbx_serial_settings.Location = New System.Drawing.Point(12, 31)
        Me.grbx_serial_settings.Name = "grbx_serial_settings"
        Me.grbx_serial_settings.Size = New System.Drawing.Size(803, 65)
        Me.grbx_serial_settings.TabIndex = 5
        Me.grbx_serial_settings.TabStop = False
        Me.grbx_serial_settings.Text = "Serial Port Settings"
        '
        'btn_701xReset
        '
        Me.btn_701xReset.Location = New System.Drawing.Point(679, 26)
        Me.btn_701xReset.Name = "btn_701xReset"
        Me.btn_701xReset.Size = New System.Drawing.Size(91, 23)
        Me.btn_701xReset.TabIndex = 8
        Me.btn_701xReset.Text = "GV701x Reset"
        Me.btn_701xReset.UseVisualStyleBackColor = True
        '
        'chkbx_rts
        '
        Me.chkbx_rts.AutoSize = True
        Me.chkbx_rts.Checked = True
        Me.chkbx_rts.CheckState = System.Windows.Forms.CheckState.Checked
        Me.chkbx_rts.Location = New System.Drawing.Point(537, 31)
        Me.chkbx_rts.Name = "chkbx_rts"
        Me.chkbx_rts.Size = New System.Drawing.Size(133, 17)
        Me.chkbx_rts.TabIndex = 7
        Me.chkbx_rts.Text = "DTR to Reset GV701x"
        Me.chkbx_rts.UseVisualStyleBackColor = True
        '
        'btn_portSearch
        '
        Me.btn_portSearch.Location = New System.Drawing.Point(175, 27)
        Me.btn_portSearch.Name = "btn_portSearch"
        Me.btn_portSearch.Size = New System.Drawing.Size(75, 23)
        Me.btn_portSearch.TabIndex = 6
        Me.btn_portSearch.Text = "Search"
        Me.btn_portSearch.UseVisualStyleBackColor = True
        '
        'btn_serialConnect
        '
        Me.btn_serialConnect.Location = New System.Drawing.Point(427, 26)
        Me.btn_serialConnect.Name = "btn_serialConnect"
        Me.btn_serialConnect.Size = New System.Drawing.Size(75, 23)
        Me.btn_serialConnect.TabIndex = 5
        Me.btn_serialConnect.Text = "Connect"
        Me.btn_serialConnect.UseVisualStyleBackColor = True
        '
        'combobx_portNo
        '
        Me.combobx_portNo.FormattingEnabled = True
        Me.combobx_portNo.Location = New System.Drawing.Point(78, 28)
        Me.combobx_portNo.Name = "combobx_portNo"
        Me.combobx_portNo.Size = New System.Drawing.Size(85, 21)
        Me.combobx_portNo.TabIndex = 4
        '
        'combobx_baudRate
        '
        Me.combobx_baudRate.FormattingEnabled = True
        Me.combobx_baudRate.Items.AddRange(New Object() {"115200"})
        Me.combobx_baudRate.Location = New System.Drawing.Point(339, 28)
        Me.combobx_baudRate.Name = "combobx_baudRate"
        Me.combobx_baudRate.Size = New System.Drawing.Size(72, 21)
        Me.combobx_baudRate.TabIndex = 3
        Me.combobx_baudRate.Text = "115200"
        '
        'label3
        '
        Me.label3.AutoSize = True
        Me.label3.Location = New System.Drawing.Point(272, 31)
        Me.label3.Name = "label3"
        Me.label3.Size = New System.Drawing.Size(61, 13)
        Me.label3.TabIndex = 2
        Me.label3.Text = "Baud Rate:"
        '
        'label2
        '
        Me.label2.AutoSize = True
        Me.label2.Location = New System.Drawing.Point(26, 31)
        Me.label2.Name = "label2"
        Me.label2.Size = New System.Drawing.Size(46, 13)
        Me.label2.TabIndex = 1
        Me.label2.Text = "Port No."
        '
        'tab_ctrl_flashmode
        '
        Me.tab_ctrl_flashmode.Controls.Add(Me.tabPage1)
        Me.tab_ctrl_flashmode.Controls.Add(Me.tab_config)
        Me.tab_ctrl_flashmode.Controls.Add(Me.TabPage2)
        Me.tab_ctrl_flashmode.Location = New System.Drawing.Point(12, 100)
        Me.tab_ctrl_flashmode.Name = "tab_ctrl_flashmode"
        Me.tab_ctrl_flashmode.SelectedIndex = 0
        Me.tab_ctrl_flashmode.Size = New System.Drawing.Size(807, 485)
        Me.tab_ctrl_flashmode.TabIndex = 6
        '
        'tabPage1
        '
        Me.tabPage1.Controls.Add(Me.Label5)
        Me.tabPage1.Controls.Add(Me.lbl_filename)
        Me.tabPage1.Controls.Add(Me.grbx_flashOption)
        Me.tabPage1.Controls.Add(Me.grbx_console)
        Me.tabPage1.Controls.Add(Me.grbx_hexfilepath)
        Me.tabPage1.Location = New System.Drawing.Point(4, 22)
        Me.tabPage1.Name = "tabPage1"
        Me.tabPage1.Padding = New System.Windows.Forms.Padding(3)
        Me.tabPage1.Size = New System.Drawing.Size(799, 459)
        Me.tabPage1.TabIndex = 0
        Me.tabPage1.Text = "Flash Mode"
        Me.tabPage1.UseVisualStyleBackColor = True
        '
        'Label5
        '
        Me.Label5.AutoSize = True
        Me.Label5.Location = New System.Drawing.Point(530, 28)
        Me.Label5.Name = "Label5"
        Me.Label5.Size = New System.Drawing.Size(57, 13)
        Me.Label5.TabIndex = 8
        Me.Label5.Text = "File in use:"
        '
        'lbl_filename
        '
        Me.lbl_filename.AutoSize = True
        Me.lbl_filename.Location = New System.Drawing.Point(530, 45)
        Me.lbl_filename.Name = "lbl_filename"
        Me.lbl_filename.Size = New System.Drawing.Size(10, 13)
        Me.lbl_filename.TabIndex = 7
        Me.lbl_filename.Text = ":"
        '
        'grbx_flashOption
        '
        Me.grbx_flashOption.Controls.Add(Me.chkbx_boot_backup)
        Me.grbx_flashOption.Controls.Add(Me.Label10)
        Me.grbx_flashOption.Controls.Add(Me.chk_bx_load_to_cram)
        Me.grbx_flashOption.Controls.Add(Me.btn_abort)
        Me.grbx_flashOption.Controls.Add(Me.ProgressBar1)
        Me.grbx_flashOption.Controls.Add(Me.btn_autoFlash)
        Me.grbx_flashOption.Controls.Add(Me.groupBox5)
        Me.grbx_flashOption.Location = New System.Drawing.Point(661, 6)
        Me.grbx_flashOption.Name = "grbx_flashOption"
        Me.grbx_flashOption.Size = New System.Drawing.Size(124, 446)
        Me.grbx_flashOption.TabIndex = 6
        Me.grbx_flashOption.TabStop = False
        Me.grbx_flashOption.Text = "Flash Options"
        '
        'Label10
        '
        Me.Label10.AutoSize = True
        Me.Label10.Location = New System.Drawing.Point(15, 144)
        Me.Label10.Margin = New System.Windows.Forms.Padding(2, 0, 2, 0)
        Me.Label10.Name = "Label10"
        Me.Label10.Size = New System.Drawing.Size(98, 13)
        Me.Label10.TabIndex = 9
        Me.Label10.Text = "Only for developers"
        '
        'chk_bx_load_to_cram
        '
        Me.chk_bx_load_to_cram.AutoSize = True
        Me.chk_bx_load_to_cram.Location = New System.Drawing.Point(5, 164)
        Me.chk_bx_load_to_cram.Name = "chk_bx_load_to_cram"
        Me.chk_bx_load_to_cram.Size = New System.Drawing.Size(124, 17)
        Me.chk_bx_load_to_cram.TabIndex = 8
        Me.chk_bx_load_to_cram.Text = "Load Code to CRAM"
        Me.chk_bx_load_to_cram.UseVisualStyleBackColor = True
        '
        'btn_abort
        '
        Me.btn_abort.Enabled = False
        Me.btn_abort.Location = New System.Drawing.Point(24, 84)
        Me.btn_abort.Name = "btn_abort"
        Me.btn_abort.Size = New System.Drawing.Size(75, 32)
        Me.btn_abort.TabIndex = 3
        Me.btn_abort.Text = "Abort"
        Me.btn_abort.UseVisualStyleBackColor = True
        '
        'ProgressBar1
        '
        Me.ProgressBar1.Location = New System.Drawing.Point(13, 18)
        Me.ProgressBar1.Maximum = 80
        Me.ProgressBar1.Name = "ProgressBar1"
        Me.ProgressBar1.Size = New System.Drawing.Size(100, 23)
        Me.ProgressBar1.Style = System.Windows.Forms.ProgressBarStyle.Continuous
        Me.ProgressBar1.TabIndex = 7
        '
        'btn_autoFlash
        '
        Me.btn_autoFlash.Location = New System.Drawing.Point(24, 47)
        Me.btn_autoFlash.Name = "btn_autoFlash"
        Me.btn_autoFlash.Size = New System.Drawing.Size(75, 32)
        Me.btn_autoFlash.TabIndex = 2
        Me.btn_autoFlash.Text = "Auto Flash"
        Me.btn_autoFlash.UseVisualStyleBackColor = True
        '
        'groupBox5
        '
        Me.groupBox5.Controls.Add(Me.radiobtn_customBank)
        Me.groupBox5.Controls.Add(Me.radiobtn_allBank)
        Me.groupBox5.Controls.Add(Me.chkbx_b7)
        Me.groupBox5.Controls.Add(Me.chkbx_b6)
        Me.groupBox5.Controls.Add(Me.chkbx_b5)
        Me.groupBox5.Controls.Add(Me.chkbx_b4)
        Me.groupBox5.Controls.Add(Me.chkbx_b3)
        Me.groupBox5.Controls.Add(Me.chkbx_b2)
        Me.groupBox5.Controls.Add(Me.chkbx_b1)
        Me.groupBox5.Controls.Add(Me.chkbx_b0)
        Me.groupBox5.Location = New System.Drawing.Point(16, 187)
        Me.groupBox5.Name = "groupBox5"
        Me.groupBox5.Size = New System.Drawing.Size(92, 247)
        Me.groupBox5.TabIndex = 1
        Me.groupBox5.TabStop = False
        Me.groupBox5.Text = "Bank Option"
        '
        'radiobtn_customBank
        '
        Me.radiobtn_customBank.AutoSize = True
        Me.radiobtn_customBank.Location = New System.Drawing.Point(14, 48)
        Me.radiobtn_customBank.Name = "radiobtn_customBank"
        Me.radiobtn_customBank.Size = New System.Drawing.Size(60, 17)
        Me.radiobtn_customBank.TabIndex = 9
        Me.radiobtn_customBank.Text = "Custom"
        Me.radiobtn_customBank.UseVisualStyleBackColor = True
        '
        'radiobtn_allBank
        '
        Me.radiobtn_allBank.AutoSize = True
        Me.radiobtn_allBank.Checked = True
        Me.radiobtn_allBank.Location = New System.Drawing.Point(14, 24)
        Me.radiobtn_allBank.Name = "radiobtn_allBank"
        Me.radiobtn_allBank.Size = New System.Drawing.Size(68, 17)
        Me.radiobtn_allBank.TabIndex = 8
        Me.radiobtn_allBank.TabStop = True
        Me.radiobtn_allBank.Text = "All banks"
        Me.radiobtn_allBank.UseVisualStyleBackColor = True
        '
        'chkbx_b7
        '
        Me.chkbx_b7.AutoSize = True
        Me.chkbx_b7.Checked = True
        Me.chkbx_b7.CheckState = System.Windows.Forms.CheckState.Checked
        Me.chkbx_b7.Location = New System.Drawing.Point(16, 225)
        Me.chkbx_b7.Name = "chkbx_b7"
        Me.chkbx_b7.Size = New System.Drawing.Size(60, 17)
        Me.chkbx_b7.TabIndex = 7
        Me.chkbx_b7.Text = "Bank 7"
        Me.chkbx_b7.UseVisualStyleBackColor = True
        '
        'chkbx_b6
        '
        Me.chkbx_b6.AutoSize = True
        Me.chkbx_b6.Checked = True
        Me.chkbx_b6.CheckState = System.Windows.Forms.CheckState.Checked
        Me.chkbx_b6.Location = New System.Drawing.Point(16, 203)
        Me.chkbx_b6.Name = "chkbx_b6"
        Me.chkbx_b6.Size = New System.Drawing.Size(60, 17)
        Me.chkbx_b6.TabIndex = 6
        Me.chkbx_b6.Text = "Bank 6"
        Me.chkbx_b6.UseVisualStyleBackColor = True
        '
        'chkbx_b5
        '
        Me.chkbx_b5.AutoSize = True
        Me.chkbx_b5.Checked = True
        Me.chkbx_b5.CheckState = System.Windows.Forms.CheckState.Checked
        Me.chkbx_b5.Location = New System.Drawing.Point(16, 182)
        Me.chkbx_b5.Name = "chkbx_b5"
        Me.chkbx_b5.Size = New System.Drawing.Size(60, 17)
        Me.chkbx_b5.TabIndex = 5
        Me.chkbx_b5.Text = "Bank 5"
        Me.chkbx_b5.UseVisualStyleBackColor = True
        '
        'chkbx_b4
        '
        Me.chkbx_b4.AutoSize = True
        Me.chkbx_b4.Checked = True
        Me.chkbx_b4.CheckState = System.Windows.Forms.CheckState.Checked
        Me.chkbx_b4.Location = New System.Drawing.Point(16, 161)
        Me.chkbx_b4.Name = "chkbx_b4"
        Me.chkbx_b4.Size = New System.Drawing.Size(60, 17)
        Me.chkbx_b4.TabIndex = 4
        Me.chkbx_b4.Text = "Bank 4"
        Me.chkbx_b4.UseVisualStyleBackColor = True
        '
        'chkbx_b3
        '
        Me.chkbx_b3.AutoSize = True
        Me.chkbx_b3.Checked = True
        Me.chkbx_b3.CheckState = System.Windows.Forms.CheckState.Checked
        Me.chkbx_b3.Location = New System.Drawing.Point(16, 140)
        Me.chkbx_b3.Name = "chkbx_b3"
        Me.chkbx_b3.Size = New System.Drawing.Size(60, 17)
        Me.chkbx_b3.TabIndex = 3
        Me.chkbx_b3.Text = "Bank 3"
        Me.chkbx_b3.UseVisualStyleBackColor = True
        '
        'chkbx_b2
        '
        Me.chkbx_b2.AutoSize = True
        Me.chkbx_b2.Checked = True
        Me.chkbx_b2.CheckState = System.Windows.Forms.CheckState.Checked
        Me.chkbx_b2.Location = New System.Drawing.Point(16, 118)
        Me.chkbx_b2.Name = "chkbx_b2"
        Me.chkbx_b2.Size = New System.Drawing.Size(60, 17)
        Me.chkbx_b2.TabIndex = 2
        Me.chkbx_b2.Text = "Bank 2"
        Me.chkbx_b2.UseVisualStyleBackColor = True
        '
        'chkbx_b1
        '
        Me.chkbx_b1.AutoSize = True
        Me.chkbx_b1.Checked = True
        Me.chkbx_b1.CheckState = System.Windows.Forms.CheckState.Checked
        Me.chkbx_b1.Location = New System.Drawing.Point(16, 97)
        Me.chkbx_b1.Name = "chkbx_b1"
        Me.chkbx_b1.Size = New System.Drawing.Size(60, 17)
        Me.chkbx_b1.TabIndex = 1
        Me.chkbx_b1.Text = "Bank 1"
        Me.chkbx_b1.UseVisualStyleBackColor = True
        '
        'chkbx_b0
        '
        Me.chkbx_b0.AutoSize = True
        Me.chkbx_b0.Checked = True
        Me.chkbx_b0.CheckState = System.Windows.Forms.CheckState.Checked
        Me.chkbx_b0.Location = New System.Drawing.Point(16, 76)
        Me.chkbx_b0.Name = "chkbx_b0"
        Me.chkbx_b0.Size = New System.Drawing.Size(60, 17)
        Me.chkbx_b0.TabIndex = 0
        Me.chkbx_b0.Text = "Bank 0"
        Me.chkbx_b0.UseVisualStyleBackColor = True
        '
        'grbx_console
        '
        Me.grbx_console.Controls.Add(Me.txtbx_console)
        Me.grbx_console.Location = New System.Drawing.Point(14, 100)
        Me.grbx_console.Name = "grbx_console"
        Me.grbx_console.Size = New System.Drawing.Size(632, 352)
        Me.grbx_console.TabIndex = 5
        Me.grbx_console.TabStop = False
        Me.grbx_console.Text = "Console"
        '
        'txtbx_console
        '
        Me.txtbx_console.AcceptsTab = True
        Me.txtbx_console.BackColor = System.Drawing.SystemColors.WindowText
        Me.txtbx_console.Font = New System.Drawing.Font("Courier New", 9.75!)
        Me.txtbx_console.ForeColor = System.Drawing.Color.White
        Me.txtbx_console.Location = New System.Drawing.Point(9, 19)
        Me.txtbx_console.MaxLength = 60000
        Me.txtbx_console.Multiline = True
        Me.txtbx_console.Name = "txtbx_console"
        Me.txtbx_console.ScrollBars = System.Windows.Forms.ScrollBars.Vertical
        Me.txtbx_console.Size = New System.Drawing.Size(609, 325)
        Me.txtbx_console.TabIndex = 0
        '
        'grbx_hexfilepath
        '
        Me.grbx_hexfilepath.Controls.Add(Me.label1)
        Me.grbx_hexfilepath.Controls.Add(Me.txtbx_fileName)
        Me.grbx_hexfilepath.Controls.Add(Me.txtbx_filePath)
        Me.grbx_hexfilepath.Controls.Add(Me.btn_pathBrowse)
        Me.grbx_hexfilepath.Location = New System.Drawing.Point(13, 10)
        Me.grbx_hexfilepath.Name = "grbx_hexfilepath"
        Me.grbx_hexfilepath.Size = New System.Drawing.Size(506, 84)
        Me.grbx_hexfilepath.TabIndex = 4
        Me.grbx_hexfilepath.TabStop = False
        Me.grbx_hexfilepath.Text = "Hexfile Path"
        '
        'label1
        '
        Me.label1.AutoSize = True
        Me.label1.Location = New System.Drawing.Point(6, 53)
        Me.label1.Name = "label1"
        Me.label1.Size = New System.Drawing.Size(84, 13)
        Me.label1.TabIndex = 3
        Me.label1.Text = "File Initial Name:"
        '
        'txtbx_fileName
        '
        Me.txtbx_fileName.Location = New System.Drawing.Point(93, 50)
        Me.txtbx_fileName.Name = "txtbx_fileName"
        Me.txtbx_fileName.Size = New System.Drawing.Size(148, 20)
        Me.txtbx_fileName.TabIndex = 2
        '
        'txtbx_filePath
        '
        Me.txtbx_filePath.Location = New System.Drawing.Point(6, 20)
        Me.txtbx_filePath.Name = "txtbx_filePath"
        Me.txtbx_filePath.Size = New System.Drawing.Size(391, 20)
        Me.txtbx_filePath.TabIndex = 0
        '
        'btn_pathBrowse
        '
        Me.btn_pathBrowse.Location = New System.Drawing.Point(403, 18)
        Me.btn_pathBrowse.Name = "btn_pathBrowse"
        Me.btn_pathBrowse.Size = New System.Drawing.Size(75, 23)
        Me.btn_pathBrowse.TabIndex = 0
        Me.btn_pathBrowse.Text = "Browse"
        Me.btn_pathBrowse.UseVisualStyleBackColor = True
        '
        'tab_config
        '
        Me.tab_config.Controls.Add(Me.btn_device_type)
        Me.tab_config.Controls.Add(Me.txtbx_device_type)
        Me.tab_config.Controls.Add(Me.Label8)
        Me.tab_config.Controls.Add(Me.btn_cin_clear)
        Me.tab_config.Controls.Add(Me.txtbx_cin_clear)
        Me.tab_config.Controls.Add(Me.Label7)
        Me.tab_config.Controls.Add(Me.lbl_key_length)
        Me.tab_config.Controls.Add(Me.btn_commit)
        Me.tab_config.Controls.Add(Me.btn_send_key)
        Me.tab_config.Controls.Add(Me.btn_send_mac)
        Me.tab_config.Controls.Add(Me.txtbx_defKey)
        Me.tab_config.Controls.Add(Me.txtbx_mac_addr)
        Me.tab_config.Controls.Add(Me.Label6)
        Me.tab_config.Controls.Add(Me.Label4)
        Me.tab_config.Location = New System.Drawing.Point(4, 22)
        Me.tab_config.Name = "tab_config"
        Me.tab_config.Padding = New System.Windows.Forms.Padding(3)
        Me.tab_config.Size = New System.Drawing.Size(799, 459)
        Me.tab_config.TabIndex = 1
        Me.tab_config.Text = "Config"
        Me.tab_config.UseVisualStyleBackColor = True
        '
        'btn_device_type
        '
        Me.btn_device_type.Location = New System.Drawing.Point(559, 124)
        Me.btn_device_type.Name = "btn_device_type"
        Me.btn_device_type.Size = New System.Drawing.Size(84, 23)
        Me.btn_device_type.TabIndex = 13
        Me.btn_device_type.Text = "Device Type"
        Me.btn_device_type.UseVisualStyleBackColor = True
        '
        'txtbx_device_type
        '
        Me.txtbx_device_type.Font = New System.Drawing.Font("Microsoft Sans Serif", 9.0!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.txtbx_device_type.Location = New System.Drawing.Point(245, 124)
        Me.txtbx_device_type.MaxLength = 100
        Me.txtbx_device_type.Name = "txtbx_device_type"
        Me.txtbx_device_type.Size = New System.Drawing.Size(277, 21)
        Me.txtbx_device_type.TabIndex = 12
        Me.txtbx_device_type.Text = "p app 11 cfg 4 1 1 1 1 0 0"
        '
        'Label8
        '
        Me.Label8.AutoSize = True
        Me.Label8.Font = New System.Drawing.Font("Microsoft Sans Serif", 9.0!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.Label8.Location = New System.Drawing.Point(15, 127)
        Me.Label8.Name = "Label8"
        Me.Label8.Size = New System.Drawing.Size(84, 15)
        Me.Label8.TabIndex = 11
        Me.Label8.Text = "Device Type"
        '
        'btn_cin_clear
        '
        Me.btn_cin_clear.Location = New System.Drawing.Point(559, 153)
        Me.btn_cin_clear.Name = "btn_cin_clear"
        Me.btn_cin_clear.Size = New System.Drawing.Size(84, 23)
        Me.btn_cin_clear.TabIndex = 10
        Me.btn_cin_clear.Text = "Clear Cin"
        Me.btn_cin_clear.UseVisualStyleBackColor = True
        '
        'txtbx_cin_clear
        '
        Me.txtbx_cin_clear.Font = New System.Drawing.Font("Microsoft Sans Serif", 9.0!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.txtbx_cin_clear.Location = New System.Drawing.Point(245, 155)
        Me.txtbx_cin_clear.MaxLength = 32
        Me.txtbx_cin_clear.Name = "txtbx_cin_clear"
        Me.txtbx_cin_clear.ReadOnly = True
        Me.txtbx_cin_clear.Size = New System.Drawing.Size(191, 21)
        Me.txtbx_cin_clear.TabIndex = 9
        Me.txtbx_cin_clear.Text = "p app 6 nvclear"
        '
        'Label7
        '
        Me.Label7.AutoSize = True
        Me.Label7.Font = New System.Drawing.Font("Microsoft Sans Serif", 9.0!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.Label7.Location = New System.Drawing.Point(15, 158)
        Me.Label7.Name = "Label7"
        Me.Label7.Size = New System.Drawing.Size(148, 15)
        Me.Label7.TabIndex = 8
        Me.Label7.Text = "Commision Cin delete"
        '
        'lbl_key_length
        '
        Me.lbl_key_length.AutoSize = True
        Me.lbl_key_length.Location = New System.Drawing.Point(529, 98)
        Me.lbl_key_length.Name = "lbl_key_length"
        Me.lbl_key_length.Size = New System.Drawing.Size(13, 13)
        Me.lbl_key_length.TabIndex = 7
        Me.lbl_key_length.Text = "0"
        '
        'btn_commit
        '
        Me.btn_commit.Location = New System.Drawing.Point(658, 66)
        Me.btn_commit.Name = "btn_commit"
        Me.btn_commit.Size = New System.Drawing.Size(114, 81)
        Me.btn_commit.TabIndex = 6
        Me.btn_commit.Text = "Commit Changes"
        Me.btn_commit.UseVisualStyleBackColor = True
        '
        'btn_send_key
        '
        Me.btn_send_key.Location = New System.Drawing.Point(559, 95)
        Me.btn_send_key.Name = "btn_send_key"
        Me.btn_send_key.Size = New System.Drawing.Size(84, 23)
        Me.btn_send_key.TabIndex = 5
        Me.btn_send_key.Text = "Send Key"
        Me.btn_send_key.UseVisualStyleBackColor = True
        '
        'btn_send_mac
        '
        Me.btn_send_mac.Location = New System.Drawing.Point(559, 65)
        Me.btn_send_mac.Name = "btn_send_mac"
        Me.btn_send_mac.Size = New System.Drawing.Size(84, 23)
        Me.btn_send_mac.TabIndex = 4
        Me.btn_send_mac.Text = "Send MAC"
        Me.btn_send_mac.UseVisualStyleBackColor = True
        '
        'txtbx_defKey
        '
        Me.txtbx_defKey.Font = New System.Drawing.Font("Microsoft Sans Serif", 9.0!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.txtbx_defKey.Location = New System.Drawing.Point(245, 95)
        Me.txtbx_defKey.MaxLength = 32
        Me.txtbx_defKey.Name = "txtbx_defKey"
        Me.txtbx_defKey.Size = New System.Drawing.Size(277, 21)
        Me.txtbx_defKey.TabIndex = 3
        '
        'txtbx_mac_addr
        '
        Me.txtbx_mac_addr.Font = New System.Drawing.Font("Microsoft Sans Serif", 9.75!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.txtbx_mac_addr.Location = New System.Drawing.Point(245, 66)
        Me.txtbx_mac_addr.MaxLength = 17
        Me.txtbx_mac_addr.Name = "txtbx_mac_addr"
        Me.txtbx_mac_addr.Size = New System.Drawing.Size(191, 22)
        Me.txtbx_mac_addr.TabIndex = 2
        Me.txtbx_mac_addr.Text = "84:86:F3:"
        '
        'Label6
        '
        Me.Label6.AutoSize = True
        Me.Label6.Font = New System.Drawing.Font("Microsoft Sans Serif", 9.0!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.Label6.Location = New System.Drawing.Point(15, 98)
        Me.Label6.Name = "Label6"
        Me.Label6.Size = New System.Drawing.Size(220, 15)
        Me.Label6.TabIndex = 1
        Me.Label6.Text = "Default Key (Primary Key - pdkey)"
        '
        'Label4
        '
        Me.Label4.AutoSize = True
        Me.Label4.Font = New System.Drawing.Font("Microsoft Sans Serif", 9.75!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.Label4.Location = New System.Drawing.Point(15, 72)
        Me.Label4.Name = "Label4"
        Me.Label4.Size = New System.Drawing.Size(106, 16)
        Me.Label4.TabIndex = 0
        Me.Label4.Text = "MAC Address:"
        '
        'TabPage2
        '
        Me.TabPage2.Controls.Add(Me.btn_save_macro)
        Me.TabPage2.Controls.Add(Me.txtbx_macro9)
        Me.TabPage2.Controls.Add(Me.Label16)
        Me.TabPage2.Controls.Add(Me.txtbx_macro8)
        Me.TabPage2.Controls.Add(Me.Label17)
        Me.TabPage2.Controls.Add(Me.txtbx_macro7)
        Me.TabPage2.Controls.Add(Me.Label18)
        Me.TabPage2.Controls.Add(Me.txtbx_macro6)
        Me.TabPage2.Controls.Add(Me.Label19)
        Me.TabPage2.Controls.Add(Me.txtbx_macro5)
        Me.TabPage2.Controls.Add(Me.Label20)
        Me.TabPage2.Controls.Add(Me.txtbx_macro4)
        Me.TabPage2.Controls.Add(Me.Label15)
        Me.TabPage2.Controls.Add(Me.txtbx_macro3)
        Me.TabPage2.Controls.Add(Me.Label14)
        Me.TabPage2.Controls.Add(Me.txtbx_macro2)
        Me.TabPage2.Controls.Add(Me.Label13)
        Me.TabPage2.Controls.Add(Me.txtbx_macro1)
        Me.TabPage2.Controls.Add(Me.Label12)
        Me.TabPage2.Controls.Add(Me.txtbx_macro0)
        Me.TabPage2.Controls.Add(Me.Label11)
        Me.TabPage2.Controls.Add(Me.Label9)
        Me.TabPage2.Location = New System.Drawing.Point(4, 22)
        Me.TabPage2.Name = "TabPage2"
        Me.TabPage2.Padding = New System.Windows.Forms.Padding(3)
        Me.TabPage2.Size = New System.Drawing.Size(799, 459)
        Me.TabPage2.TabIndex = 2
        Me.TabPage2.Text = "Macro"
        Me.TabPage2.UseVisualStyleBackColor = True
        '
        'btn_save_macro
        '
        Me.btn_save_macro.Font = New System.Drawing.Font("Microsoft Sans Serif", 8.25!, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.btn_save_macro.Location = New System.Drawing.Point(601, 144)
        Me.btn_save_macro.Name = "btn_save_macro"
        Me.btn_save_macro.Size = New System.Drawing.Size(122, 100)
        Me.btn_save_macro.TabIndex = 20
        Me.btn_save_macro.Text = "Save Macro"
        Me.btn_save_macro.UseVisualStyleBackColor = True
        '
        'txtbx_macro9
        '
        Me.txtbx_macro9.Font = New System.Drawing.Font("Microsoft Sans Serif", 8.25!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.txtbx_macro9.Location = New System.Drawing.Point(74, 331)
        Me.txtbx_macro9.Name = "txtbx_macro9"
        Me.txtbx_macro9.Size = New System.Drawing.Size(418, 20)
        Me.txtbx_macro9.TabIndex = 10
        '
        'Label16
        '
        Me.Label16.AutoSize = True
        Me.Label16.Font = New System.Drawing.Font("Microsoft Sans Serif", 8.25!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.Label16.Location = New System.Drawing.Point(42, 334)
        Me.Label16.Name = "Label16"
        Me.Label16.Size = New System.Drawing.Size(14, 13)
        Me.Label16.TabIndex = 19
        Me.Label16.Text = "9"
        '
        'txtbx_macro8
        '
        Me.txtbx_macro8.Font = New System.Drawing.Font("Microsoft Sans Serif", 8.25!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.txtbx_macro8.Location = New System.Drawing.Point(74, 301)
        Me.txtbx_macro8.Name = "txtbx_macro8"
        Me.txtbx_macro8.Size = New System.Drawing.Size(418, 20)
        Me.txtbx_macro8.TabIndex = 9
        '
        'Label17
        '
        Me.Label17.AutoSize = True
        Me.Label17.Font = New System.Drawing.Font("Microsoft Sans Serif", 8.25!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.Label17.Location = New System.Drawing.Point(42, 304)
        Me.Label17.Name = "Label17"
        Me.Label17.Size = New System.Drawing.Size(14, 13)
        Me.Label17.TabIndex = 17
        Me.Label17.Text = "8"
        '
        'txtbx_macro7
        '
        Me.txtbx_macro7.Font = New System.Drawing.Font("Microsoft Sans Serif", 8.25!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.txtbx_macro7.Location = New System.Drawing.Point(74, 271)
        Me.txtbx_macro7.Name = "txtbx_macro7"
        Me.txtbx_macro7.Size = New System.Drawing.Size(418, 20)
        Me.txtbx_macro7.TabIndex = 8
        '
        'Label18
        '
        Me.Label18.AutoSize = True
        Me.Label18.Font = New System.Drawing.Font("Microsoft Sans Serif", 8.25!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.Label18.Location = New System.Drawing.Point(42, 274)
        Me.Label18.Name = "Label18"
        Me.Label18.Size = New System.Drawing.Size(14, 13)
        Me.Label18.TabIndex = 15
        Me.Label18.Text = "7"
        '
        'txtbx_macro6
        '
        Me.txtbx_macro6.Font = New System.Drawing.Font("Microsoft Sans Serif", 8.25!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.txtbx_macro6.Location = New System.Drawing.Point(74, 241)
        Me.txtbx_macro6.Name = "txtbx_macro6"
        Me.txtbx_macro6.Size = New System.Drawing.Size(418, 20)
        Me.txtbx_macro6.TabIndex = 7
        '
        'Label19
        '
        Me.Label19.AutoSize = True
        Me.Label19.Font = New System.Drawing.Font("Microsoft Sans Serif", 8.25!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.Label19.Location = New System.Drawing.Point(42, 244)
        Me.Label19.Name = "Label19"
        Me.Label19.Size = New System.Drawing.Size(14, 13)
        Me.Label19.TabIndex = 13
        Me.Label19.Text = "6"
        '
        'txtbx_macro5
        '
        Me.txtbx_macro5.Font = New System.Drawing.Font("Microsoft Sans Serif", 8.25!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.txtbx_macro5.Location = New System.Drawing.Point(74, 212)
        Me.txtbx_macro5.Name = "txtbx_macro5"
        Me.txtbx_macro5.Size = New System.Drawing.Size(418, 20)
        Me.txtbx_macro5.TabIndex = 6
        '
        'Label20
        '
        Me.Label20.AutoSize = True
        Me.Label20.Font = New System.Drawing.Font("Microsoft Sans Serif", 8.25!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.Label20.Location = New System.Drawing.Point(42, 215)
        Me.Label20.Name = "Label20"
        Me.Label20.Size = New System.Drawing.Size(14, 13)
        Me.Label20.TabIndex = 11
        Me.Label20.Text = "5"
        '
        'txtbx_macro4
        '
        Me.txtbx_macro4.Font = New System.Drawing.Font("Microsoft Sans Serif", 8.25!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.txtbx_macro4.Location = New System.Drawing.Point(74, 182)
        Me.txtbx_macro4.Name = "txtbx_macro4"
        Me.txtbx_macro4.Size = New System.Drawing.Size(418, 20)
        Me.txtbx_macro4.TabIndex = 5
        '
        'Label15
        '
        Me.Label15.AutoSize = True
        Me.Label15.Font = New System.Drawing.Font("Microsoft Sans Serif", 8.25!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.Label15.Location = New System.Drawing.Point(42, 185)
        Me.Label15.Name = "Label15"
        Me.Label15.Size = New System.Drawing.Size(14, 13)
        Me.Label15.TabIndex = 9
        Me.Label15.Text = "4"
        '
        'txtbx_macro3
        '
        Me.txtbx_macro3.Font = New System.Drawing.Font("Microsoft Sans Serif", 8.25!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.txtbx_macro3.Location = New System.Drawing.Point(74, 152)
        Me.txtbx_macro3.Name = "txtbx_macro3"
        Me.txtbx_macro3.Size = New System.Drawing.Size(418, 20)
        Me.txtbx_macro3.TabIndex = 4
        '
        'Label14
        '
        Me.Label14.AutoSize = True
        Me.Label14.Font = New System.Drawing.Font("Microsoft Sans Serif", 8.25!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.Label14.Location = New System.Drawing.Point(42, 155)
        Me.Label14.Name = "Label14"
        Me.Label14.Size = New System.Drawing.Size(14, 13)
        Me.Label14.TabIndex = 7
        Me.Label14.Text = "3"
        '
        'txtbx_macro2
        '
        Me.txtbx_macro2.Font = New System.Drawing.Font("Microsoft Sans Serif", 8.25!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.txtbx_macro2.Location = New System.Drawing.Point(74, 122)
        Me.txtbx_macro2.Name = "txtbx_macro2"
        Me.txtbx_macro2.Size = New System.Drawing.Size(418, 20)
        Me.txtbx_macro2.TabIndex = 3
        '
        'Label13
        '
        Me.Label13.AutoSize = True
        Me.Label13.Font = New System.Drawing.Font("Microsoft Sans Serif", 8.25!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.Label13.Location = New System.Drawing.Point(42, 125)
        Me.Label13.Name = "Label13"
        Me.Label13.Size = New System.Drawing.Size(14, 13)
        Me.Label13.TabIndex = 5
        Me.Label13.Text = "2"
        '
        'txtbx_macro1
        '
        Me.txtbx_macro1.Font = New System.Drawing.Font("Microsoft Sans Serif", 8.25!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.txtbx_macro1.Location = New System.Drawing.Point(74, 92)
        Me.txtbx_macro1.Name = "txtbx_macro1"
        Me.txtbx_macro1.Size = New System.Drawing.Size(418, 20)
        Me.txtbx_macro1.TabIndex = 2
        '
        'Label12
        '
        Me.Label12.AutoSize = True
        Me.Label12.Font = New System.Drawing.Font("Microsoft Sans Serif", 8.25!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.Label12.Location = New System.Drawing.Point(42, 95)
        Me.Label12.Name = "Label12"
        Me.Label12.Size = New System.Drawing.Size(14, 13)
        Me.Label12.TabIndex = 3
        Me.Label12.Text = "1"
        '
        'txtbx_macro0
        '
        Me.txtbx_macro0.Font = New System.Drawing.Font("Microsoft Sans Serif", 8.25!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.txtbx_macro0.Location = New System.Drawing.Point(74, 63)
        Me.txtbx_macro0.Name = "txtbx_macro0"
        Me.txtbx_macro0.Size = New System.Drawing.Size(418, 20)
        Me.txtbx_macro0.TabIndex = 1
        Me.txtbx_macro0.Visible = False
        '
        'Label11
        '
        Me.Label11.AutoSize = True
        Me.Label11.Font = New System.Drawing.Font("Microsoft Sans Serif", 8.25!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.Label11.Location = New System.Drawing.Point(42, 66)
        Me.Label11.Name = "Label11"
        Me.Label11.Size = New System.Drawing.Size(14, 13)
        Me.Label11.TabIndex = 1
        Me.Label11.Text = "0"
        Me.Label11.Visible = False
        '
        'Label9
        '
        Me.Label9.AutoSize = True
        Me.Label9.Font = New System.Drawing.Font("Microsoft Sans Serif", 8.25!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.Label9.Location = New System.Drawing.Point(39, 22)
        Me.Label9.Name = "Label9"
        Me.Label9.Size = New System.Drawing.Size(447, 13)
        Me.Label9.TabIndex = 0
        Me.Label9.Text = "Press CTRL + SHIFT + 1-9 to transmit specific user defined macro to firmware"
        '
        'SerialPort1
        '
        Me.SerialPort1.WriteBufferSize = 80000
        '
        'MenuStrip1
        '
        Me.MenuStrip1.BackColor = System.Drawing.SystemColors.Control
        Me.MenuStrip1.ImageScalingSize = New System.Drawing.Size(20, 20)
        Me.MenuStrip1.Items.AddRange(New System.Windows.Forms.ToolStripItem() {Me.FileToolStripMenuItem, Me.EditToolStripMenuItem, Me.ToolsToolStripMenuItem, Me.WindowToolStripMenuItem, Me.HelpToolStripMenuItem})
        Me.MenuStrip1.Location = New System.Drawing.Point(0, 0)
        Me.MenuStrip1.Name = "MenuStrip1"
        Me.MenuStrip1.Size = New System.Drawing.Size(821, 24)
        Me.MenuStrip1.TabIndex = 7
        Me.MenuStrip1.Text = "MenuStrip1"
        '
        'FileToolStripMenuItem
        '
        Me.FileToolStripMenuItem.DropDownItems.AddRange(New System.Windows.Forms.ToolStripItem() {Me.toolStripSeparator, Me.toolStripSeparator1, Me.toolStripSeparator2, Me.ExitToolStripMenuItem})
        Me.FileToolStripMenuItem.Name = "FileToolStripMenuItem"
        Me.FileToolStripMenuItem.Size = New System.Drawing.Size(37, 20)
        Me.FileToolStripMenuItem.Text = "&File"
        '
        'toolStripSeparator
        '
        Me.toolStripSeparator.Name = "toolStripSeparator"
        Me.toolStripSeparator.Size = New System.Drawing.Size(89, 6)
        '
        'toolStripSeparator1
        '
        Me.toolStripSeparator1.Name = "toolStripSeparator1"
        Me.toolStripSeparator1.Size = New System.Drawing.Size(89, 6)
        '
        'toolStripSeparator2
        '
        Me.toolStripSeparator2.Name = "toolStripSeparator2"
        Me.toolStripSeparator2.Size = New System.Drawing.Size(89, 6)
        '
        'ExitToolStripMenuItem
        '
        Me.ExitToolStripMenuItem.Name = "ExitToolStripMenuItem"
        Me.ExitToolStripMenuItem.Size = New System.Drawing.Size(92, 22)
        Me.ExitToolStripMenuItem.Text = "E&xit"
        '
        'EditToolStripMenuItem
        '
        Me.EditToolStripMenuItem.DropDownItems.AddRange(New System.Windows.Forms.ToolStripItem() {Me.toolStripSeparator3, Me.toolStripSeparator4, Me.ConsoleClearToolStripMenuItem})
        Me.EditToolStripMenuItem.Name = "EditToolStripMenuItem"
        Me.EditToolStripMenuItem.Size = New System.Drawing.Size(39, 20)
        Me.EditToolStripMenuItem.Text = "&Edit"
        '
        'toolStripSeparator3
        '
        Me.toolStripSeparator3.Name = "toolStripSeparator3"
        Me.toolStripSeparator3.Size = New System.Drawing.Size(144, 6)
        '
        'toolStripSeparator4
        '
        Me.toolStripSeparator4.Name = "toolStripSeparator4"
        Me.toolStripSeparator4.Size = New System.Drawing.Size(144, 6)
        '
        'ConsoleClearToolStripMenuItem
        '
        Me.ConsoleClearToolStripMenuItem.Name = "ConsoleClearToolStripMenuItem"
        Me.ConsoleClearToolStripMenuItem.Size = New System.Drawing.Size(147, 22)
        Me.ConsoleClearToolStripMenuItem.Text = "Console Clear"
        '
        'ToolsToolStripMenuItem
        '
        Me.ToolsToolStripMenuItem.DropDownItems.AddRange(New System.Windows.Forms.ToolStripItem() {Me.ExpandToolStripMenuItem_file_path, Me.CustomizeToolStripMenuItem, Me.OptionsToolStripMenuItem, Me.PortConnectToolStripMenuItem, Me.ToolStripSeparator5, Me.ToolStripSeparator6, Me.ShrinkToolStripMenuItem_autoflash, Me.AbortToolStripMenuItem_abort})
        Me.ToolsToolStripMenuItem.Name = "ToolsToolStripMenuItem"
        Me.ToolsToolStripMenuItem.Size = New System.Drawing.Size(48, 20)
        Me.ToolsToolStripMenuItem.Text = "&Tools"
        '
        'ExpandToolStripMenuItem_file_path
        '
        Me.ExpandToolStripMenuItem_file_path.Name = "ExpandToolStripMenuItem_file_path"
        Me.ExpandToolStripMenuItem_file_path.Size = New System.Drawing.Size(164, 22)
        Me.ExpandToolStripMenuItem_file_path.Text = "File Path"
        '
        'CustomizeToolStripMenuItem
        '
        Me.CustomizeToolStripMenuItem.Name = "CustomizeToolStripMenuItem"
        Me.CustomizeToolStripMenuItem.Size = New System.Drawing.Size(164, 22)
        Me.CustomizeToolStripMenuItem.Text = "Customize Banks"
        '
        'OptionsToolStripMenuItem
        '
        Me.OptionsToolStripMenuItem.Name = "OptionsToolStripMenuItem"
        Me.OptionsToolStripMenuItem.Size = New System.Drawing.Size(164, 22)
        Me.OptionsToolStripMenuItem.Text = "Options"
        '
        'PortConnectToolStripMenuItem
        '
        Me.PortConnectToolStripMenuItem.Name = "PortConnectToolStripMenuItem"
        Me.PortConnectToolStripMenuItem.Size = New System.Drawing.Size(164, 22)
        Me.PortConnectToolStripMenuItem.Text = "Port Connect"
        '
        'ToolStripSeparator5
        '
        Me.ToolStripSeparator5.Name = "ToolStripSeparator5"
        Me.ToolStripSeparator5.Size = New System.Drawing.Size(161, 6)
        '
        'ToolStripSeparator6
        '
        Me.ToolStripSeparator6.Name = "ToolStripSeparator6"
        Me.ToolStripSeparator6.Size = New System.Drawing.Size(161, 6)
        '
        'ShrinkToolStripMenuItem_autoflash
        '
        Me.ShrinkToolStripMenuItem_autoflash.Name = "ShrinkToolStripMenuItem_autoflash"
        Me.ShrinkToolStripMenuItem_autoflash.Size = New System.Drawing.Size(164, 22)
        Me.ShrinkToolStripMenuItem_autoflash.Text = "Auto Flash"
        '
        'AbortToolStripMenuItem_abort
        '
        Me.AbortToolStripMenuItem_abort.Name = "AbortToolStripMenuItem_abort"
        Me.AbortToolStripMenuItem_abort.Size = New System.Drawing.Size(164, 22)
        Me.AbortToolStripMenuItem_abort.Text = "Abort"
        '
        'WindowToolStripMenuItem
        '
        Me.WindowToolStripMenuItem.DropDownItems.AddRange(New System.Windows.Forms.ToolStripItem() {Me.ShrinkToolStripMenuItem1, Me.ExpandToolStripMenuItem1})
        Me.WindowToolStripMenuItem.Name = "WindowToolStripMenuItem"
        Me.WindowToolStripMenuItem.Size = New System.Drawing.Size(63, 20)
        Me.WindowToolStripMenuItem.Text = "Window"
        '
        'ShrinkToolStripMenuItem1
        '
        Me.ShrinkToolStripMenuItem1.Name = "ShrinkToolStripMenuItem1"
        Me.ShrinkToolStripMenuItem1.Size = New System.Drawing.Size(112, 22)
        Me.ShrinkToolStripMenuItem1.Text = "Shrink"
        '
        'ExpandToolStripMenuItem1
        '
        Me.ExpandToolStripMenuItem1.Name = "ExpandToolStripMenuItem1"
        Me.ExpandToolStripMenuItem1.Size = New System.Drawing.Size(112, 22)
        Me.ExpandToolStripMenuItem1.Text = "Expand"
        '
        'HelpToolStripMenuItem
        '
        Me.HelpToolStripMenuItem.DropDownItems.AddRange(New System.Windows.Forms.ToolStripItem() {Me.ShortcutKeysReferenceToolStripMenuItem})
        Me.HelpToolStripMenuItem.Name = "HelpToolStripMenuItem"
        Me.HelpToolStripMenuItem.Size = New System.Drawing.Size(44, 20)
        Me.HelpToolStripMenuItem.Text = "Help"
        '
        'ShortcutKeysReferenceToolStripMenuItem
        '
        Me.ShortcutKeysReferenceToolStripMenuItem.Name = "ShortcutKeysReferenceToolStripMenuItem"
        Me.ShortcutKeysReferenceToolStripMenuItem.Size = New System.Drawing.Size(201, 22)
        Me.ShortcutKeysReferenceToolStripMenuItem.Text = "Shortcut Keys Reference"
        '
        'FileSystemWatcher1
        '
        Me.FileSystemWatcher1.EnableRaisingEvents = True
        Me.FileSystemWatcher1.SynchronizingObject = Me
        '
        'chkbx_boot_backup
        '
        Me.chkbx_boot_backup.AutoSize = True
        Me.chkbx_boot_backup.Location = New System.Drawing.Point(5, 124)
        Me.chkbx_boot_backup.Name = "chkbx_boot_backup"
        Me.chkbx_boot_backup.Size = New System.Drawing.Size(122, 17)
        Me.chkbx_boot_backup.TabIndex = 10
        Me.chkbx_boot_backup.Text = "Flash Boot+ Backup"
        Me.chkbx_boot_backup.UseVisualStyleBackColor = True
        '
        'Form1
        '
        Me.AutoScaleDimensions = New System.Drawing.SizeF(6.0!, 13.0!)
        Me.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font
        Me.ClientSize = New System.Drawing.Size(821, 585)
        Me.Controls.Add(Me.tab_ctrl_flashmode)
        Me.Controls.Add(Me.grbx_serial_settings)
        Me.Controls.Add(Me.MenuStrip1)
        Me.Icon = CType(resources.GetObject("$this.Icon"), System.Drawing.Icon)
        Me.Name = "Form1"
        Me.StartPosition = System.Windows.Forms.FormStartPosition.CenterScreen
        Me.Text = "Flashing Utility"
        Me.grbx_serial_settings.ResumeLayout(False)
        Me.grbx_serial_settings.PerformLayout()
        Me.tab_ctrl_flashmode.ResumeLayout(False)
        Me.tabPage1.ResumeLayout(False)
        Me.tabPage1.PerformLayout()
        Me.grbx_flashOption.ResumeLayout(False)
        Me.grbx_flashOption.PerformLayout()
        Me.groupBox5.ResumeLayout(False)
        Me.groupBox5.PerformLayout()
        Me.grbx_console.ResumeLayout(False)
        Me.grbx_console.PerformLayout()
        Me.grbx_hexfilepath.ResumeLayout(False)
        Me.grbx_hexfilepath.PerformLayout()
        Me.tab_config.ResumeLayout(False)
        Me.tab_config.PerformLayout()
        Me.TabPage2.ResumeLayout(False)
        Me.TabPage2.PerformLayout()
        Me.MenuStrip1.ResumeLayout(False)
        Me.MenuStrip1.PerformLayout()
        CType(Me.FileSystemWatcher1, System.ComponentModel.ISupportInitialize).EndInit()
        Me.ResumeLayout(False)
        Me.PerformLayout()

    End Sub
    Private WithEvents grbx_serial_settings As System.Windows.Forms.GroupBox
    Private WithEvents btn_serialConnect As System.Windows.Forms.Button
    Private WithEvents combobx_portNo As System.Windows.Forms.ComboBox
    Private WithEvents combobx_baudRate As System.Windows.Forms.ComboBox
    Private WithEvents label3 As System.Windows.Forms.Label
    Private WithEvents label2 As System.Windows.Forms.Label
    Private WithEvents tab_ctrl_flashmode As System.Windows.Forms.TabControl
    Private WithEvents tabPage1 As System.Windows.Forms.TabPage
    Private WithEvents grbx_flashOption As System.Windows.Forms.GroupBox
    Private WithEvents btn_autoFlash As System.Windows.Forms.Button
    Private WithEvents groupBox5 As System.Windows.Forms.GroupBox
    Private WithEvents chkbx_b7 As System.Windows.Forms.CheckBox
    Private WithEvents chkbx_b6 As System.Windows.Forms.CheckBox
    Private WithEvents chkbx_b5 As System.Windows.Forms.CheckBox
    Private WithEvents chkbx_b4 As System.Windows.Forms.CheckBox
    Private WithEvents chkbx_b3 As System.Windows.Forms.CheckBox
    Private WithEvents chkbx_b2 As System.Windows.Forms.CheckBox
    Private WithEvents chkbx_b1 As System.Windows.Forms.CheckBox
    Private WithEvents chkbx_b0 As System.Windows.Forms.CheckBox
    Private WithEvents grbx_console As System.Windows.Forms.GroupBox
    Private WithEvents grbx_hexfilepath As System.Windows.Forms.GroupBox
    Private WithEvents label1 As System.Windows.Forms.Label
    Private WithEvents txtbx_fileName As System.Windows.Forms.TextBox
    Private WithEvents txtbx_filePath As System.Windows.Forms.TextBox
    Private WithEvents btn_pathBrowse As System.Windows.Forms.Button
    Private WithEvents btn_portSearch As System.Windows.Forms.Button
    Friend WithEvents SerialPort1 As System.IO.Ports.SerialPort
    Friend WithEvents FolderBrowserDialog1 As System.Windows.Forms.FolderBrowserDialog
    Friend WithEvents MenuStrip1 As System.Windows.Forms.MenuStrip
    Friend WithEvents FileToolStripMenuItem As System.Windows.Forms.ToolStripMenuItem
    Friend WithEvents toolStripSeparator As System.Windows.Forms.ToolStripSeparator
    Friend WithEvents toolStripSeparator1 As System.Windows.Forms.ToolStripSeparator
    Friend WithEvents toolStripSeparator2 As System.Windows.Forms.ToolStripSeparator
    Friend WithEvents ExitToolStripMenuItem As System.Windows.Forms.ToolStripMenuItem
    Friend WithEvents EditToolStripMenuItem As System.Windows.Forms.ToolStripMenuItem
    Friend WithEvents toolStripSeparator3 As System.Windows.Forms.ToolStripSeparator
    Friend WithEvents toolStripSeparator4 As System.Windows.Forms.ToolStripSeparator
    Friend WithEvents ToolsToolStripMenuItem As System.Windows.Forms.ToolStripMenuItem
    Friend WithEvents CustomizeToolStripMenuItem As System.Windows.Forms.ToolStripMenuItem
    Friend WithEvents OptionsToolStripMenuItem As System.Windows.Forms.ToolStripMenuItem
    Friend WithEvents ShrinkToolStripMenuItem_autoflash As System.Windows.Forms.ToolStripMenuItem
    Friend WithEvents ExpandToolStripMenuItem_file_path As System.Windows.Forms.ToolStripMenuItem
    Friend WithEvents WindowToolStripMenuItem As System.Windows.Forms.ToolStripMenuItem
    Friend WithEvents ShrinkToolStripMenuItem1 As System.Windows.Forms.ToolStripMenuItem
    Friend WithEvents ExpandToolStripMenuItem1 As System.Windows.Forms.ToolStripMenuItem
    Friend WithEvents ProgressBar1 As System.Windows.Forms.ProgressBar
    Private WithEvents btn_abort As System.Windows.Forms.Button
    Friend WithEvents chkbx_rts As System.Windows.Forms.CheckBox
    Private WithEvents btn_701xReset As System.Windows.Forms.Button
    Friend WithEvents OpenFileDialog1 As System.Windows.Forms.OpenFileDialog
    Friend WithEvents SaveFileDialog1 As System.Windows.Forms.SaveFileDialog
    Friend WithEvents AbortToolStripMenuItem_abort As System.Windows.Forms.ToolStripMenuItem
    Friend WithEvents Label5 As System.Windows.Forms.Label
    Friend WithEvents lbl_filename As System.Windows.Forms.Label
    Friend WithEvents PortConnectToolStripMenuItem As System.Windows.Forms.ToolStripMenuItem
    Friend WithEvents ConsoleClearToolStripMenuItem As System.Windows.Forms.ToolStripMenuItem
    Public WithEvents txtbx_console As System.Windows.Forms.TextBox
    Private WithEvents radiobtn_customBank As RadioButton
    Private WithEvents radiobtn_allBank As RadioButton
    Private WithEvents chk_bx_load_to_cram As CheckBox
    Friend WithEvents Label10 As Label
    Friend WithEvents ToolStripSeparator5 As ToolStripSeparator
    Friend WithEvents ToolStripSeparator6 As ToolStripSeparator
    Friend WithEvents tab_config As TabPage
    Friend WithEvents FileSystemWatcher1 As IO.FileSystemWatcher
    Friend WithEvents Label6 As Label
    Friend WithEvents Label4 As Label
    Friend WithEvents btn_send_key As Button
    Friend WithEvents btn_send_mac As Button
    Friend WithEvents txtbx_defKey As TextBox
    Friend WithEvents txtbx_mac_addr As TextBox
    Friend WithEvents btn_commit As Button
    Friend WithEvents lbl_key_length As Label
    Friend WithEvents txtbx_cin_clear As TextBox
    Friend WithEvents Label7 As Label
    Friend WithEvents btn_cin_clear As Button
    Friend WithEvents btn_device_type As Button
    Friend WithEvents txtbx_device_type As TextBox
    Friend WithEvents Label8 As Label
    Friend WithEvents HelpToolStripMenuItem As ToolStripMenuItem
    Friend WithEvents ShortcutKeysReferenceToolStripMenuItem As ToolStripMenuItem
    Friend WithEvents TabPage2 As TabPage
    Friend WithEvents Label9 As Label
    Friend WithEvents txtbx_macro9 As TextBox
    Friend WithEvents Label16 As Label
    Friend WithEvents txtbx_macro8 As TextBox
    Friend WithEvents Label17 As Label
    Friend WithEvents txtbx_macro7 As TextBox
    Friend WithEvents Label18 As Label
    Friend WithEvents txtbx_macro6 As TextBox
    Friend WithEvents Label19 As Label
    Friend WithEvents txtbx_macro5 As TextBox
    Friend WithEvents Label20 As Label
    Friend WithEvents txtbx_macro4 As TextBox
    Friend WithEvents Label15 As Label
    Friend WithEvents txtbx_macro3 As TextBox
    Friend WithEvents Label14 As Label
    Friend WithEvents txtbx_macro2 As TextBox
    Friend WithEvents Label13 As Label
    Friend WithEvents txtbx_macro1 As TextBox
    Friend WithEvents Label12 As Label
    Friend WithEvents txtbx_macro0 As TextBox
    Friend WithEvents Label11 As Label
    Friend WithEvents btn_save_macro As Button
    Friend WithEvents chkbx_boot_backup As CheckBox
End Class
