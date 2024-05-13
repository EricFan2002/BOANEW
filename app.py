import pandas as pd
import plotly.graph_objects as go
from plotly.subplots import make_subplots
import dash
import dash_core_components as dcc
import dash_html_components as html
from dash.dependencies import Input, Output

# Read CSV data
exchange_report_df = pd.read_csv('output_exchange_report.csv')
client_report_df = pd.read_csv('output_client_report.csv')
instrument_report_df = pd.read_csv('output_instrument_report.csv')

# Create a Dash app
app = dash.Dash(__name__)

# Define the layout
app.layout = html.Div([
    html.H1("Trading Dashboard"),
    dcc.Tabs([
        dcc.Tab(label="Exchange Report", children=[
            html.Div([
                dcc.Dropdown(
                    id='exchange-dropdown',
                    options=[{'label': order_id, 'value': order_id} for order_id in exchange_report_df['OrderID'].unique()],
                    placeholder="Select an Order ID"
                ),
                html.Div(id='exchange-chart')
            ])
        ]),
        dcc.Tab(label="Client Report", children=[
            html.Div([
                dcc.Dropdown(
                    id='client-dropdown',
                    options=[{'label': client_id, 'value': client_id} for client_id in client_report_df['ClientID'].unique()],
                    placeholder="Select a Client ID"
                ),
                html.Div(id='client-chart')
            ])
        ]),
        dcc.Tab(label="Instrument Report", children=[
            html.Div([
                dcc.Dropdown(
                    id='instrument-dropdown',
                    options=[{'label': instrument_id, 'value': instrument_id} for instrument_id in instrument_report_df['InstrumentID'].unique()],
                    placeholder="Select an Instrument ID"
                ),
                html.Div(id='instrument-chart')
            ])
        ])
    ])
])

# Callback for Exchange Report
@app.callback(Output('exchange-chart', 'children'),
              [Input('exchange-dropdown', 'value')])
def update_exchange_chart(order_id):
    if order_id is None:
        return None

    # Filter data based on selected Order ID
    filtered_df = exchange_report_df[exchange_report_df['OrderID'] == order_id]

    # Create chart based on filtered data
    chart = go.Figure(data=[go.Table(
        header=dict(values=["OrderID", "RejectionReason"]),
        cells=dict(values=[filtered_df['OrderID'], filtered_df['RejectionReason']])
    )])

    return dcc.Graph(figure=chart)

# Callback for Client Report
@app.callback(Output('client-chart', 'children'),
              [Input('client-dropdown', 'value')])
def update_client_chart(client_id):
    if client_id is None:
        return None

    # Filter data based on selected Client ID
    filtered_df = client_report_df[client_report_df['ClientID'] == client_id]

    # Create chart based on filtered data
    chart = go.Figure(data=[go.Table(
        header=dict(values=["ClientID", "InstrumentID", "NetPosition"]),
        cells=dict(values=[filtered_df['ClientID'], filtered_df['InstrumentID'], filtered_df['NetPosition']])
    )])

    return dcc.Graph(figure=chart)

# Callback for Instrument Report
@app.callback(Output('instrument-chart', 'children'),
              [Input('instrument-dropdown', 'value')])
def update_instrument_chart(instrument_id):
    if instrument_id is None:
        return None

    # Filter data based on selected Instrument ID
    filtered_df = instrument_report_df[instrument_report_df['InstrumentID'] == instrument_id]

    # Create chart based on filtered data
    chart = go.Figure(data=[go.Table(
        header=dict(values=["InstrumentID", "OpenPrice", "ClosePrice", "TotalVolume", "VWAP", "DayHigh", "DayLow"]),
        cells=dict(values=[
            filtered_df['InstrumentID'],
            filtered_df['OpenPrice'],
            filtered_df['ClosePrice'],
            filtered_df['TotalVolume'],
            filtered_df['VWAP'],
            filtered_df['DayHigh'],
            filtered_df['DayLow']
        ])
    )])

    return dcc.Graph(figure=chart)

if __name__ == '__main__':
    app.run_server(debug=True)