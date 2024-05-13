import pandas as pd
import plotly.graph_objects as go
from plotly.subplots import make_subplots

# Read CSV data
exchange_report_df = pd.read_csv('output_exchange_report.csv')
client_report_df = pd.read_csv('output_client_report.csv')
instrument_report_df = pd.read_csv('output_instrument_report.csv')

# Create subplots with domain type for tables
fig = make_subplots(
    rows=3, cols=1,
    subplot_titles=("Exchange Report", "Client Report", "Instrument Report"),
    specs=[[{"type": "domain"}], [{"type": "domain"}], [{"type": "domain"}]]
)

# Exchange Report
exchange_table = go.Table(
    header=dict(values=["OrderID", "RejectionReason"]),
    cells=dict(values=[exchange_report_df['OrderID'], exchange_report_df['RejectionReason']])
)
fig.add_trace(exchange_table, row=1, col=1)

# Client Report
client_table = go.Table(
    header=dict(values=["ClientID", "InstrumentID", "NetPosition"]),
    cells=dict(values=[client_report_df['ClientID'], client_report_df['InstrumentID'], client_report_df['NetPosition']])
)
fig.add_trace(client_table, row=2, col=1)

# Instrument Report
instrument_table = go.Table(
    header=dict(values=["InstrumentID", "OpenPrice", "ClosePrice", "TotalVolume", "VWAP", "DayHigh", "DayLow"]),
    cells=dict(values=[
        instrument_report_df['InstrumentID'],
        instrument_report_df['OpenPrice'],
        instrument_report_df['ClosePrice'],
        instrument_report_df['TotalVolume'],
        instrument_report_df['VWAP'],
        instrument_report_df['DayHigh'],
        instrument_report_df['DayLow']
    ])
)
fig.add_trace(instrument_table, row=3, col=1)

# Update layout
fig.update_layout(
    height=800,
    showlegend=False,
    title_text="Trading Dashboard",
    template="plotly_white"
)

# Show the dashboard
fig.show()