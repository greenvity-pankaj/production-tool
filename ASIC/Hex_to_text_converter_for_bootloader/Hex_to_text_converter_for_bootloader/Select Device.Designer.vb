<Global.Microsoft.VisualBasic.CompilerServices.DesignerGenerated()> _
Partial Class Select_Device
    Inherits System.Windows.Forms.Form

    'Form overrides dispose to clean up the component list.
    <System.Diagnostics.DebuggerNonUserCode()> _
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
    <System.Diagnostics.DebuggerStepThrough()> _
    Private Sub InitializeComponent()
        'Me.At_isp_device = New USB_Atmel_Device_ISP_Programmer.at_isp_device
        Me.btn_select = New System.Windows.Forms.Button
        Me.TextBox1 = New System.Windows.Forms.TextBox
        'CType(Me.At_isp_device, System.ComponentModel.ISupportInitialize).BeginInit()
        Me.SuspendLayout()
        '
        'At_isp_device
        '
        ' Me.At_isp_device.DataSetName = "at_isp_device"
        ' Me.At_isp_device.SchemaSerializationMode = System.Data.SchemaSerializationMode.IncludeSchema
        '
        'btn_select
        '
        Me.btn_select.Location = New System.Drawing.Point(451, 43)
        Me.btn_select.Name = "btn_select"
        Me.btn_select.Size = New System.Drawing.Size(75, 23)
        Me.btn_select.TabIndex = 0
        Me.btn_select.Text = "Select"
        Me.btn_select.UseVisualStyleBackColor = True
        '
        'TextBox1
        '
        Me.TextBox1.Location = New System.Drawing.Point(53, 110)
        Me.TextBox1.Multiline = True
        Me.TextBox1.Name = "TextBox1"
        Me.TextBox1.ScrollBars = System.Windows.Forms.ScrollBars.Vertical
        Me.TextBox1.Size = New System.Drawing.Size(434, 179)
        Me.TextBox1.TabIndex = 1
        '
        'Select_Device
        '
        Me.AutoScaleDimensions = New System.Drawing.SizeF(6.0!, 13.0!)
        Me.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font
        Me.ClientSize = New System.Drawing.Size(746, 381)
        Me.Controls.Add(Me.TextBox1)
        Me.Controls.Add(Me.btn_select)
        Me.Name = "Select_Device"
        Me.Text = "Select_Device"
        ' CType(Me.At_isp_device, System.ComponentModel.ISupportInitialize).EndInit()
        Me.ResumeLayout(False)
        Me.PerformLayout()

    End Sub
    'Friend WithEvents At_isp_device As USB_Atmel_Device_ISP_Programmer.at_isp_device
    Friend WithEvents btn_select As System.Windows.Forms.Button
    Friend WithEvents TextBox1 As System.Windows.Forms.TextBox
End Class
